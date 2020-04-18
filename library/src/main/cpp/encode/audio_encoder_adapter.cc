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

#include "audio_encoder_adapter.h"
#include "android_xlog.h"

namespace trinity {

AudioEncoderAdapter::AudioEncoderAdapter() {
    encoding_ = false;
    audio_encoder_ = nullptr;
    pcm_packet_pool_ = nullptr;
    aac_packet_pool_ = nullptr;
    packet_buffer_size_ = 0;
    packet_buffer_ = nullptr;
    packet_buffer_cursor_ = 0;
    audio_sample_rate_ = 0;
    audio_channels_ = 0;
    audio_bit_rate_ = 0;
    audio_codec_name_ = nullptr;
    packet_buffer_presentation_time_mills_ = 0;
}

AudioEncoderAdapter::~AudioEncoderAdapter() {
}

void AudioEncoderAdapter::Init(PacketPool *pool, int audio_sample_rate, int audio_channels, int audio_bit_rate,
                               const char *audio_codec_name) {
    LOGE("enter %s", __func__);
    packet_buffer_ = new short[4096];
    packet_buffer_size_ = 0;
    packet_buffer_cursor_ = 0;
    pcm_packet_pool_ = pool;
    audio_sample_rate_ = audio_sample_rate;
    audio_channels_ = audio_channels;
    audio_bit_rate_ = audio_bit_rate;
    auto audio_codec_name_length = strlen(audio_codec_name);
    audio_codec_name_ = new char[audio_codec_name_length + 1];
    memset(audio_codec_name_, 0, audio_codec_name_length + 1);
    memcpy(audio_codec_name_, audio_codec_name, audio_codec_name_length);
    encoding_ = true;
    aac_packet_pool_ = AudioPacketPool::GetInstance();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&audio_encoder_thread_, &attr, StartEncodeThread, this);
    LOGE("leave %s", __func__);
}

int AudioEncoderAdapter::GetAudioFrame(int16_t *samples, int frame_size,
        int nb_channels, double *presentation_time_mills) {
    int size = frame_size * nb_channels * 2;
    int sample_cursor = 0;
    while (true) {
        if (packet_buffer_size_ == 0) {
            int ret = GetAudioPacket();
            if (ret < 0) {
                return ret;
            }
        }

        int samples_short_size = (size - sample_cursor * 2) / 2;
        if (packet_buffer_cursor_ + samples_short_size <= packet_buffer_size_) {
            CopyToSamples(samples, sample_cursor, samples_short_size, presentation_time_mills);
            packet_buffer_cursor_ += samples_short_size;
            break;
        } else {
            int packet_buffer_size = packet_buffer_size_ - packet_buffer_cursor_;
            CopyToSamples(samples, sample_cursor, packet_buffer_size, presentation_time_mills);
            sample_cursor += packet_buffer_size;
            packet_buffer_size_ = 0;
            continue;
        }
    }
    return frame_size * nb_channels;
}

void AudioEncoderAdapter::Destroy() {
    LOGE("enter %s", __func__);
    if (!encoding_) {
        return;
    }
    encoding_ = false;
    pcm_packet_pool_->AbortAudioPacketQueue();
    pthread_join(audio_encoder_thread_, nullptr);
    if (nullptr != audio_codec_name_) {
        delete[] audio_codec_name_;
        audio_codec_name_ = nullptr;
    }
    if (nullptr != packet_buffer_) {
        delete[] packet_buffer_;
        packet_buffer_ = nullptr;
    }
    LOGE("leave %s", __func__);
}

void *AudioEncoderAdapter::StartEncodeThread(void *context) {
    auto* adapter = reinterpret_cast<AudioEncoderAdapter*>(context);
    adapter->StartEncode();
    pthread_exit(nullptr);
}

static int PCMFrameCallback(int16_t *samples, int frame_size, int nb_channels, double *presentationTimeMills,
                            void *context) {
    auto* adapter = reinterpret_cast<AudioEncoderAdapter*>(context);
    return adapter->GetAudioFrame(samples, frame_size, nb_channels, presentationTimeMills);
}

void AudioEncoderAdapter::StartEncode() {
    audio_encoder_ = new AudioEncoder();
    audio_encoder_->Init(audio_bit_rate_, audio_channels_,
            audio_sample_rate_, audio_codec_name_, PCMFrameCallback, this);
    while (encoding_) {
        audio_encoder_->Encode(aac_packet_pool_);
    }
    pcm_packet_pool_->DestroyAudioPacketQueue();
    if (nullptr != audio_encoder_) {
        audio_encoder_->Destroy();
        delete audio_encoder_;
        audio_encoder_ = nullptr;
    }
}

int AudioEncoderAdapter::CopyToSamples(int16_t *samples, int sample_cursor, int buffer_size,
                                       double *presentation_time_mills) {
    if (0 == sample_cursor) {
        double packet_buffer_cursor_duration = static_cast<double>(packet_buffer_cursor_)
                                   * 1000.0 / (audio_sample_rate_ * 2.0);
        (*presentation_time_mills) = packet_buffer_presentation_time_mills_ + packet_buffer_cursor_duration;
    }
    memcpy(samples + sample_cursor, packet_buffer_ + packet_buffer_cursor_, buffer_size * sizeof(short));
    return 1;
}

int AudioEncoderAdapter::GetAudioPacket() {
    if (!encoding_) {
        return -1;
    }
//    this->DiscardAudioPacket();
    AudioPacket *audioPacket = nullptr;
    if (pcm_packet_pool_->GetAccompanyPacketQueueSize() != 0) {
        pcm_packet_pool_->GetAccompanyPacket(&audioPacket, true);
    } else {
        if (pcm_packet_pool_->GetAudioPacket(&audioPacket, true) < 0) {
            return -1;
        }
    }
    packet_buffer_cursor_ = 0;
    packet_buffer_presentation_time_mills_ = audioPacket->position;
    /**
     * 在Android平台 录制是单声道的 经过音效处理之后是双声道 channelRatio 2
     * 在iOS平台 录制的是双声道的 是已经处理音效过后的 channelRatio 1
     */
    packet_buffer_size_ = static_cast<int>((audioPacket->size * 1.0));
    memcpy(packet_buffer_, audioPacket->buffer, audioPacket->size * sizeof(short));
//    int actualSize = this->ProcessAudio();
//    if (actualSize > 0 && actualSize < packet_buffer_size_) {
//        packet_buffer_cursor_ = packet_buffer_size_ - actualSize;
//        memmove(packet_buffer_ + packet_buffer_cursor_, packet_buffer_, actualSize * sizeof(short));
//    }
    if (nullptr != audioPacket) {
        delete audioPacket;
        audioPacket = nullptr;
    }
    return packet_buffer_size_ > 0 ? 1 : -1;
}

}  // namespace trinity
