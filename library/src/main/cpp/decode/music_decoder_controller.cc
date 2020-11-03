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

enum MusicMessage {
    kMusicInit,
    kMusicStart,
    kMusicResume,
    kMusicPause,
    kMusicStop,
    kMusicDestroy
};

MusicDecoderController::MusicDecoderController()
    : running_(false)
    , lock_()
    , condition_()
    , mutex_()
    , cond_()
    , buffer_(nullptr)
    , buffer_size_(0)
    , accompany_type_(-1)
    , packet_pool_(nullptr)
    , decoder_(nullptr)
    , resample_(nullptr)
    , need_resample_(false)
    , audio_render_(nullptr)
    , accompany_sample_rate_(0)
    , silent_samples_(nullptr)
    , accompany_packet_buffer_size_(0)
    , vocal_sample_rate_(0)
    , volume_(1.0F)
    , volume_max_(1.0F)
    , decoder_thread_()
    , buffer_queue_size_(0)
    , buffer_queue_cursor_(0)
    , buffer_queue_(nullptr) {
    buffer_size_ = 2048;
    buffer_ = new uint8_t[buffer_size_];

    buffer_pool_ = new BufferPool(sizeof(Message));
    message_queue_ = new MessageQueue("Music Message Queue");
    InitMessageQueue(message_queue_);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&message_queue_thread_, &attr, MessageQueueThread, this);
}

MusicDecoderController::~MusicDecoderController() {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = MESSAGE_QUEUE_LOOP_QUIT_FLAG;
    PostMessage(message);
    pthread_join(message_queue_thread_, nullptr);

    if (nullptr != message_queue_) {
        delete message_queue_;
        message_queue_ = nullptr;
    }
    if (nullptr != buffer_) {
        delete[] buffer_;
        buffer_ = nullptr;
    }
    if (nullptr != buffer_pool_) {
        delete buffer_pool_;
        buffer_pool_ = nullptr;
    }
}

void* MusicDecoderController::MessageQueueThread(void *arg) {
    auto music_decoder_controller = reinterpret_cast<MusicDecoderController*>(arg);
    music_decoder_controller->ProcessMessage();
    pthread_exit(0);
}

void MusicDecoderController::ProcessMessage() {
    LOGI("enter %s", __func__);
    bool rendering = true;
    while (rendering) {
        Message* msg = nullptr;
        if (message_queue_->DequeueMessage(&msg, true) > 0) {
            if (nullptr != msg) {
                if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                    LOGE("MESSAGE_QUEUE_LOOP_QUIT_FLAG");
                    rendering = false;
                }
                if (nullptr != buffer_pool_) {
                    buffer_pool_->ReleaseBuffer(msg);
                }
            }
        }
    }
    LOGI("leave %s", __func__);
}

void MusicDecoderController::HandleMessage(Message *msg) {
    int what = msg->GetWhat();
    void* obj = msg->GetObj();
    switch (what) {
        case kMusicInit:
            InitWithMessage(0, msg->GetArg1());
            break;

        case kMusicStart:
            StartWithMessage(reinterpret_cast<char*>(obj));
            break;

        case kMusicResume:
            ResumeWithMessage();
            break;

        case kMusicPause:
            PauseWithMessage();
            break;

        case kMusicStop:
            StopWithMessage();
            break;

        case kMusicDestroy:
            DestroyWithMessage();
            break;

        default:
            break;
    }
}

int MusicDecoderController::AudioCallback(uint8_t** buffer, int* buffer_size, void* context) {
    auto* controller = reinterpret_cast<MusicDecoderController*>(context);
    int ret = controller->ReadSamples(controller->buffer_, controller->buffer_size_);
    if (ret < 0) {
        return ret;
    }
    *buffer = controller->buffer_;
    *buffer_size = ret;
    return 0;
}

void MusicDecoderController::Init(float packet_buffer_time_percent, int vocal_sample_rate) {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicInit;
    message->arg1 = vocal_sample_rate;
    PostMessage(message);
}

void MusicDecoderController::InitWithMessage(float packet_buffer_time_percent,
                                             int vocal_sample_rate) {
    LOGI("enter packet_buffer_time_percent: %f vocal_sample_rate: %d", packet_buffer_time_percent, vocal_sample_rate);
    volume_ = 1.0F;
    volume_max_ = 1.0F;
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
            memcpy(samples, accompanyPacket->buffer, samplePacketSize * sizeof(short));
            // push accompany packet_ to accompany queue_
            PushPacketToQueue(accompanyPacket);
            ret = samplePacketSize * sizeof(short);
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
    char* music_path = new char[strlen(path) + 1];
    strcpy(music_path, path);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicStart;
    message->obj = music_path;
    PostMessage(message);
}

void MusicDecoderController::StartWithMessage(char *path) {
    LOGI("enter: %s path: %s", __FUNCTION__, path);
    if (InitDecoder(path) >= 0) {
        pthread_mutex_lock(&mutex_);
        pthread_cond_signal(&cond_);
        pthread_mutex_unlock(&mutex_);
    }
    delete path;
    InitRender();
    if (nullptr != audio_render_) {
        audio_render_->Start();
    }
    LOGI("leave: %s", __func__);
}

void MusicDecoderController::Pause() {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicPause;
    PostMessage(message);
}

void MusicDecoderController::PauseWithMessage() {
    if (nullptr != audio_render_) {
        audio_render_->Pause();
    }
}

void MusicDecoderController::Resume() {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicResume;
    PostMessage(message);
}

void MusicDecoderController::ResumeWithMessage() {
    if (nullptr != audio_render_) {
        audio_render_->Play();
    }
}

void MusicDecoderController::Stop() {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicStop;
    PostMessage(message);
}

void MusicDecoderController::StopWithMessage() {
    LOGI("enter stop");
    if (nullptr != audio_render_) {
        audio_render_->Stop();
    }
    if (nullptr != packet_pool_) {
        packet_pool_->ClearDecoderAccompanyPacketToQueue();
    }
    DestroyResample();
    DestroyDecoder();
    DestroyRender();

    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);

    LOGI("leave stop");
}

void MusicDecoderController::Destroy() {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicDestroy;
    PostMessage(message);
}

void MusicDecoderController::DestroyWithMessage() {
    LOGI("enter MusicDecoderController::Destroy");
    if (nullptr != packet_pool_) {
        packet_pool_->AbortDecoderAccompanyPacketQueue();
        packet_pool_->DestroyDecoderAccompanyPacketQueue();
        delete packet_pool_;
        packet_pool_ = nullptr;
    }
    DestroyResample();
    DestroyDecoder();
    DestroyRender();
    if (nullptr != silent_samples_) {
        delete[] silent_samples_;
        silent_samples_ = nullptr;
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
            float ratio = accompany_sample_rate_ * 1.0F / vocal_sample_rate_;
            actualAccompanyPacketBufferSize = static_cast<int>(ratio * accompany_packet_buffer_size_);
            ret = resample_->Init(accompany_sample_rate_, vocal_sample_rate_, actualAccompanyPacketBufferSize / 2, 2, 2);
            if (ret < 0) {
                LOGE("resampler_ InitMessageQueue error\n");
            }
        } else {
            need_resample_ = false;
        }
        decoder_->SetPacketBufferSize(actualAccompanyPacketBufferSize);
    } else {
        decoder_->Destroy();
        delete decoder_;
        decoder_ = nullptr;
    }
    LOGI("leave");
    return ret;
}

int MusicDecoderController::InitRender() {
    if (nullptr == decoder_) {
        return -1;
    }
    DestroyRender();
    audio_render_ = new AudioRender();
    int sample_rate = decoder_->GetSampleRate();
    audio_render_->Init(2, sample_rate, AudioCallback, this);
    return 0;
}

void *MusicDecoderController::StartDecoderThread(void *context) {
    auto* decoderController = reinterpret_cast<MusicDecoderController*>(context);
    if (decoderController->decoder_ == nullptr) {
        pthread_mutex_lock(&decoderController->mutex_);
        pthread_cond_wait(&decoderController->cond_, &decoderController->mutex_);
        pthread_mutex_unlock(&decoderController->mutex_);
    }
    pthread_mutex_lock(&decoderController->lock_);
    while (decoderController->running_) {
        decoderController->DecodePacket();
        if (decoderController->packet_pool_->GeDecoderAccompanyPacketQueueSize() > QUEUE_SIZE_MAX_THRESHOLD) {
            decoderController->accompany_type_ = ACCOMPANY_TYPE_SONG_SAMPLE;
            pthread_cond_wait(&decoderController->condition_, &decoderController->lock_);
        }
    }
    pthread_mutex_unlock(&decoderController->lock_);
    return 0;
}

void MusicDecoderController::InitDecoderThread() {
    running_ = true;
    pthread_mutex_init(&lock_, nullptr);
    pthread_cond_init(&condition_, nullptr);
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
    pthread_create(&decoder_thread_, nullptr, StartDecoderThread, this);
}

void MusicDecoderController::DecodePacket() {
    if (nullptr == decoder_) {
        return;
    }
    AudioPacket* accompanyPacket = decoder_->DecodePacket();
    // 是否需要重采样
    if (need_resample_ && nullptr != resample_) {
        short* stereoSamples = accompanyPacket->buffer;
        int stereoSampleSize = accompanyPacket->size;
        if (stereoSampleSize > 0) {
            int monoSampleSize = stereoSampleSize / 2;
            auto** samples = new short*[2];
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
                auto* accompanySamples = new short[accompanySampleSize];
                convertShortArrayFromByteArray(out_data, out_nb_bytes, accompanySamples, 1.0);
                accompanyPacket->buffer = accompanySamples;
                accompanyPacket->size = accompanySampleSize;
            }
        }
    }
    packet_pool_->PushDecoderAccompanyPacketToQueue(accompanyPacket);
}

void MusicDecoderController::DestroyResample() {
    if (nullptr != resample_) {
        resample_->Destroy();
        delete resample_;
        resample_ = nullptr;
    }
}

void MusicDecoderController::DestroyDecoder() {
    if (nullptr != decoder_) {
        decoder_->Destroy();
        delete decoder_;
        decoder_ = nullptr;
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
        auto* buffer = new short[accompany_packet_buffer_size_];
        memcpy(buffer, buffer_queue_, accompany_packet_buffer_size_ * sizeof(short));
        int protectedSampleSize = buffer_queue_cursor_ - accompany_packet_buffer_size_;
        memmove(buffer_queue_, buffer_queue_ + accompany_packet_buffer_size_, protectedSampleSize * sizeof(short));
        buffer_queue_cursor_ -= accompany_packet_buffer_size_;
        auto* actualPacket = new AudioPacket();
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
    auto* accompanyPacket = new AudioPacket();
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
