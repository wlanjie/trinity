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

#ifndef TRINITY_AUDIO_ENCODER_ADAPTER_H
#define TRINITY_AUDIO_ENCODER_ADAPTER_H

#include "packet_pool.h"
#include "audio_encoder.h"
#include "audio_packet_pool.h"
#include <fstream>
#include <iostream>

namespace trinity {

class AudioEncoderAdapter {
 public:
    AudioEncoderAdapter();
    virtual ~AudioEncoderAdapter();
    virtual void Init(PacketPool* pool, int audio_sample_rate, int audio_channels, int audio_bit_rate, const char* audio_codec_name);
    int GetAudioFrame(int16_t* samples, int frame_size, int nb_channels, double* presentation_time_mills);
    virtual void Destroy();

 protected:
    bool encoding_;
    AudioEncoder* audio_encoder_;
    pthread_t audio_encoder_thread_;
    PacketPool* pcm_packet_pool_;
    AudioPacketPool* aac_packet_pool_;

    int packet_buffer_size_;
    short* packet_buffer_;
    int packet_buffer_cursor_;
    int audio_sample_rate_;
    int audio_channels_;
    int audio_bit_rate_;
    char* audio_codec_name_;
    double packet_buffer_presentation_time_mills_;

 protected:
    static void* StartEncodeThread(void* context);
    void StartEncode();
    int CopyToSamples(int16_t* samples, int sample_cursor, int buffer_size, double* presentation_time_mills);
    int GetAudioPacket();
};

}  // namespace trinity

#endif  // TRINITY_AUDIO_ENCODER_ADAPTER_H
