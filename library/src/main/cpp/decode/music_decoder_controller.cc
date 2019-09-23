/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by wlanjie on 2019/4/20.
//

#include <unistd.h>
#include "music_decoder_controller.h"
#include "android_xlog.h"
#include "tools.h"

namespace trinity {

MusicDecoderController::MusicDecoderController()
        : running_(false),
          accompany_type_(-1),
          suspend_flag_(false),
          packet_pool_(nullptr),
          decoder_(nullptr),
          resample_(nullptr),
          need_resample_(false),
          audio_render_(nullptr),
          accompany_sample_rate_(0),
          silent_samples_(nullptr),
          accompany_packet_buffer_size_(0),
          vocal_sample_rate_(0),
          volume_(1.0f),
          volume_max_(1.0f),
          buffer_queue_size_(0),
          buffer_queue_cursor_(0),
          buffer_queue_(nullptr) {
}

MusicDecoderController::~MusicDecoderController() {}

static int audioCallback(uint8_t* buffer, size_t buffer_size, void* context) {
    MusicDecoderController* controller = reinterpret_cast<MusicDecoderController*>(context);
    return controller->ReadSamples(buffer, 2048) * 2;
}

void MusicDecoderController::Init(float packet_buffer_time_percent, int vocal_sample_rate) {
    LOGI("enter packet_buffer_time_percent: %f vocal_sample_rate: %d", packet_buffer_time_percent, vocal_sample_rate);
    volume_ = 1.0f;
    volume_max_ = 1.0f;
    vocal_sample_rate_ = vocal_sample_rate;
    accompany_packet_buffer_size_ = 2048;
    buffer_queue_cursor_ = 0;
    buffer_queue_size_ = static_cast<int>(MAX(accompany_packet_buffer_size_ * 4,
            vocal_sample_rate * CHANNEL_PER_FRAME * 1.0f));
    buffer_queue_ = new short[buffer_queue_size_];
    silent_samples_ = new short[accompany_packet_buffer_size_];
    memset(silent_samples_, 0, accompany_packet_buffer_size_ * 2);
    packet_pool_ = new PacketPool();
    packet_pool_->InitDecoderAccompanyPacketQueue();
    packet_pool_->InitAccompanyPacketQueue(vocal_sample_rate, CHANNEL_PER_FRAME);
    InitDecoderThread();
}

int MusicDecoderController::ReadSamplesAndProducePacket(short *samples, int size, int *slient_size,
                                                        int *extra_accompany_type) {
    int result = -1;
    if (accompany_type_ == ACCOMPANY_TYPE_SILENT_SAMPLE) {
        extra_accompany_type[0] = ACCOMPANY_TYPE_SILENT_SAMPLE;
        slient_size[0] = 0;
        result = this->BuildSamples(samples);
    } else if (accompany_type_ == ACCOMPANY_TYPE_SONG_SAMPLE) {
        extra_accompany_type[0] = ACCOMPANY_TYPE_SONG_SAMPLE;
        AudioPacket* accompanyPacket = nullptr;
        packet_pool_->GetDecoderAccompanyPacket(&accompanyPacket, true);
        if (nullptr != accompanyPacket) {
            int samplePacketSize = accompanyPacket->size;
            if (samplePacketSize != -1 && samplePacketSize <= size) {
                // copy the raw data to samples
                adjustSamplesVolume(accompanyPacket->buffer, samplePacketSize, volume_ / volume_max_);
                memcpy(samples, accompanyPacket->buffer, samplePacketSize * 2);
                // push accompany packet_ to accompany queue_
                PushPacketToQueue(accompanyPacket);
                result = samplePacketSize;
            } else if (-1 == samplePacketSize) {
                LOGI("decode eof");
                // 解码到最后了
                accompany_type_ = ACCOMPANY_TYPE_SILENT_SAMPLE;
                extra_accompany_type[0] = ACCOMPANY_TYPE_SILENT_SAMPLE;
                slient_size[0] = 0;
                result = BuildSamples(samples);
            }
        } else {
            result = -2;
        }
        if (extra_accompany_type[0] != ACCOMPANY_TYPE_SILENT_SAMPLE && packet_pool_->GeDecoderAccompanyPacketQueueSize() < QUEUE_SIZE_MIN_THRESHOLD) {
            pthread_mutex_lock(&lock_);
            if (result != -1) {
                pthread_cond_signal(&condition_);
            }
            pthread_mutex_unlock(&lock_);
        }
    }
    LOGI("leave ReadSamplesAndProducePacket");
    return result;
}

int MusicDecoderController::ReadSamples(uint8_t* samples, int size) {
    int ret = -1;
    AudioPacket* accompanyPacket = nullptr;
    packet_pool_->GetDecoderAccompanyPacket(&accompanyPacket, true);
    if (nullptr != accompanyPacket) {
        int samplePacketSize = accompanyPacket->size;
        if (samplePacketSize != -1 && samplePacketSize <= size) {
            // copy the raw data to samples
            adjustSamplesVolume(accompanyPacket->buffer, samplePacketSize, volume_ / volume_max_);
            // TODO crash
            memcpy(samples, accompanyPacket->buffer, samplePacketSize * 2);
            // push accompany packet_ to accompany queue_
            PushPacketToQueue(accompanyPacket);
            ret = samplePacketSize;
        } else if (-1 == samplePacketSize) {
            LOGI("decode eof");
            // 解码到最后了
        }
    } else {
        ret = -2;
    }

    if (packet_pool_->GeDecoderAccompanyPacketQueueSize() < QUEUE_SIZE_MIN_THRESHOLD) {
        pthread_mutex_lock(&lock_);
        if (ret != -1) {
            pthread_cond_signal(&condition_);
        }
        pthread_mutex_unlock(&lock_);
    }
    return ret;
}

void MusicDecoderController::SetVolume(float volume, float volume_max) {
    volume_ = volume;
    volume_max_ = volume_max;
}

void MusicDecoderController::Start(const char* path) {
//    SuspendDecodeThread();
    usleep(200 * 1000);
    if (InitDecoder(path) >= 0) {
        ResumeDecodeThread();
    }

    InitRender();
    if (nullptr != audio_render_) {
        audio_render_->Start();
    }
}

void MusicDecoderController::Pause() {
    if (nullptr != audio_render_) {
        audio_render_->Pause();
    }
    LOGI("enter pause");
    SuspendDecodeThread();
    LOGI("leave pause");
}

void MusicDecoderController::Resume() {
    if (nullptr != audio_render_) {
        audio_render_->Play();
    }
    ResumeDecodeThread();
}

void MusicDecoderController::Stop() {
    LOGI("enter stop");
    SuspendDecodeThread();
    if (nullptr != audio_render_) {
        audio_render_->Stop();
    }
    packet_pool_->ClearDecoderAccompanyPacketToQueue();
    DestroyResample();
    DestroyDecoder();
    DestroyRender();
    LOGI("leave stop");
}

void MusicDecoderController::Destroy() {
    LOGI("enter MusicDecoderController::Destroy");
    DestroyDecoderThread();
    LOGI("after DestroyDecoderThread");
    packet_pool_->AbortDecoderAccompanyPacketQueue();
    packet_pool_->DestoryDecoderAccompanyPacketQueue();
    DestroyResample();
    DestroyDecoder();
    DestroyRender();
    if (nullptr != silent_samples_) {
        delete[] silent_samples_;
        silent_samples_ = nullptr;
    }
    if (nullptr != packet_pool_) {
        delete packet_pool_;
        packet_pool_ = nullptr;
    }
    LOGI("leave MusicDecoderController::Destroy");
}

int MusicDecoderController::InitDecoder(const char *path) {
    LOGI("enter path: %s", path);
    packet_pool_->ClearDecoderAccompanyPacketToQueue();
    DestroyResample();
    DestroyDecoder();
    decoder_ = new MusicDecoder();
    int actualAccompanyPacketBufferSize = accompany_packet_buffer_size_;
    int ret = decoder_->Init(path, accompany_packet_buffer_size_);
    if (ret >= 0) {
        // 初始化伴奏的采样率
        accompany_sample_rate_ = decoder_->GetSampleRate();
        if (vocal_sample_rate_ != accompany_sample_rate_) {
            need_resample_ = true;
            resample_ = new Resample();
            float ratio = accompany_sample_rate_ * 1.0f / vocal_sample_rate_;
            actualAccompanyPacketBufferSize = ratio * accompany_packet_buffer_size_;
            ret = resample_->Init(accompany_sample_rate_, vocal_sample_rate_, actualAccompanyPacketBufferSize / 2, 2, 2);
            if (ret < 0) {
                LOGE("resampler_ Init error\n");
            }
        } else {
            need_resample_ = false;
        }
        decoder_->SetPacketBufferSize(actualAccompanyPacketBufferSize);
    }
    LOGI("leave");
    return ret;
}

int MusicDecoderController::InitRender() {
    DestroyRender();
    audio_render_ = new AudioRender();
    int sample_rate = decoder_->GetSampleRate();
    audio_render_->Init(2, sample_rate, audioCallback, this);
    return 0;
}

void *MusicDecoderController::StartDecoderThread(void *context) {
    MusicDecoderController* decoderController = reinterpret_cast<MusicDecoderController*>(context);
    pthread_mutex_lock(&decoderController->lock_);
    while (decoderController->running_) {
        if (decoderController->suspend_flag_) {
            pthread_mutex_lock(&decoderController->suspend_lock_);
            pthread_cond_signal(&decoderController->suspend_condition_);
            pthread_mutex_unlock(&decoderController->suspend_lock_);

            pthread_cond_wait(&decoderController->condition_, &decoderController->lock_);
        } else {
            decoderController->DecodePacket();
            if (decoderController->packet_pool_->GeDecoderAccompanyPacketQueueSize() > QUEUE_SIZE_MAX_THRESHOLD) {
                decoderController->accompany_type_ = ACCOMPANY_TYPE_SONG_SAMPLE;
                pthread_cond_wait(&decoderController->condition_, &decoderController->lock_);
            }
        }
    }
    pthread_mutex_unlock(&decoderController->lock_);
    return 0;
}

void MusicDecoderController::InitDecoderThread() {
    running_ = true;
    suspend_flag_ = true;
    pthread_mutex_init(&lock_, nullptr);
    pthread_cond_init(&condition_, nullptr);
    pthread_mutex_init(&suspend_lock_, nullptr);
    pthread_cond_init(&suspend_condition_, nullptr);
    pthread_create(&decoder_thread_, nullptr, StartDecoderThread, this);
}

void MusicDecoderController::DecodePacket() {
    AudioPacket* accompanyPacket = decoder_->DecodePacket();
    // 是否需要重采样
    if (need_resample_ && NULL != resample_) {
        short* stereoSamples = accompanyPacket->buffer;
        int stereoSampleSize = accompanyPacket->size;
        if (stereoSampleSize > 0) {
            int monoSampleSize = stereoSampleSize / 2;
            short** samples = new short*[2];
            samples[0] = new short[monoSampleSize];
            samples[1] = new short[monoSampleSize];
            for (int i = 0; i < monoSampleSize; i++) {
                samples[0][i] = stereoSamples[2 * i];
                samples[1][i] = stereoSamples[2 * i + 1];
            }
            float transfer_ratio = accompany_sample_rate_ / static_cast<float>(vocal_sample_rate_);
            int accompanySampleSize = static_cast<int>(monoSampleSize * 1.0f / transfer_ratio);
            uint8_t out_data[accompanySampleSize * 2 * 2];
            int out_nb_bytes = 0;
            resample_->Process(samples, out_data, monoSampleSize, &out_nb_bytes);
            delete[] samples[0];
            delete[] samples[1];
            delete[] stereoSamples;
            if (out_nb_bytes > 0) {
                accompanySampleSize = out_nb_bytes / 2;
                short* accompanySamples = new short[accompanySampleSize];
                convertShortArrayFromByteArray(out_data, out_nb_bytes, accompanySamples, 1.0);
                accompanyPacket->buffer = accompanySamples;
                accompanyPacket->size = accompanySampleSize;
            }
        }
    }
    packet_pool_->PushDecoderAccompanyPacketToQueue(accompanyPacket);
}

void MusicDecoderController::SuspendDecodeThread() {
    pthread_mutex_lock(&suspend_lock_);
    pthread_mutex_lock(&lock_);
    suspend_flag_ = true;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    pthread_cond_wait(&suspend_condition_, &suspend_lock_);
    pthread_mutex_unlock(&suspend_lock_);
}

void MusicDecoderController::ResumeDecodeThread() {
    LOGI("enter resume");
    suspend_flag_ = false;
    pthread_mutex_lock(&lock_);
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    LOGI("leave resume");
}

void MusicDecoderController::DestroyDecoderThread() {
    running_ = false;
    suspend_flag_ = false;
    void* status;
    pthread_mutex_lock(&lock_);
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    pthread_join(decoder_thread_, &status);
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&condition_);

    pthread_mutex_lock(&suspend_lock_);
    pthread_cond_signal(&suspend_condition_);
    pthread_mutex_unlock(&suspend_lock_);
    pthread_mutex_destroy(&suspend_lock_);
    pthread_cond_destroy(&suspend_condition_);
}

void MusicDecoderController::DestroyResample() {
    if (nullptr != resample_) {
        resample_->Destroy();
        delete resample_;
        resample_ = NULL;
    }
}

void MusicDecoderController::DestroyDecoder() {
    if (nullptr != decoder_) {
        decoder_->Destroy();
        delete decoder_;
        decoder_ = NULL;
    }
}

void MusicDecoderController::DestroyRender() {
    if (nullptr != audio_render_) {
        audio_render_->DestroyContext();
        delete audio_render_;
        audio_render_ = nullptr;
    }
}

void MusicDecoderController::PushPacketToQueue(AudioPacket *packet) {
    memcpy(buffer_queue_ + buffer_queue_cursor_, packet->buffer, packet->size * sizeof(short));
    buffer_queue_cursor_ += packet->size;
    float position = packet->position;
    delete packet;
    while (buffer_queue_cursor_ >= accompany_packet_buffer_size_) {
        short* buffer = new short[accompany_packet_buffer_size_];
        memcpy(buffer, buffer_queue_, accompany_packet_buffer_size_ * sizeof(short));
        int protectedSampleSize = buffer_queue_cursor_ - accompany_packet_buffer_size_;
        memmove(buffer_queue_, buffer_queue_ + accompany_packet_buffer_size_, protectedSampleSize * sizeof(short));
        buffer_queue_cursor_ -= accompany_packet_buffer_size_;
        AudioPacket* actualPacket = new AudioPacket();
        actualPacket->size = accompany_packet_buffer_size_;
        actualPacket->buffer = buffer;
        actualPacket->position = position;
        if (position != -1) {
            actualPacket->frameNum = static_cast<long>(position * this->vocal_sample_rate_);
        } else {
            actualPacket->frameNum = -1;
        }
        packet_pool_->PushAccompanyPacketToQueue(actualPacket);
    }
}

int MusicDecoderController::BuildSamples(short *samples) {
    AudioPacket* accompanyPacket = new AudioPacket();
    int samplePacketSize = accompany_packet_buffer_size_;
    accompanyPacket->size = samplePacketSize;
    accompanyPacket->buffer = new short[samplePacketSize];
    accompanyPacket->position = -1;
    memcpy(accompanyPacket->buffer, silent_samples_, samplePacketSize * 2);
    memcpy(samples, accompanyPacket->buffer, samplePacketSize * 2);
    PushPacketToQueue(accompanyPacket);
    return samplePacketSize;
}

}  // namespace trinity
