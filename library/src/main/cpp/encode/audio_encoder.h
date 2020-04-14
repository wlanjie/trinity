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

#ifndef TRINITY_AUDIO_ENCODER_H
#define TRINITY_AUDIO_ENCODER_H

#include <stdint.h>
#include "audio_packet_queue.h"
#include "audio_packet_pool.h"

extern "C" {
#include "libavcodec/avcodec.h"
};

namespace trinity {

class AudioEncoder {
 private:
    AVCodecContext* codec_context_;
    AVFrame* encode_frame_;
    int64_t audio_next_pts_;
    uint8_t** audio_samples_data_;
    int audio_nb_frames_;
    int audio_samples_size_;
    int bit_rate_;
    int channels_;
    int sample_rate_;

    typedef int (*PCMFrameCallback)(int16_t *, int, int, double *, void *context);
    PCMFrameCallback pcm_frame_callback_;
    void *pcm_frame_context_;

 private:
    int AllocFrame();
    int AllocAudioStream(const char* codec_name);

 public:
    AudioEncoder();
    virtual ~AudioEncoder();

    int Init(int bit_rate, int channels, int sample_rate, const char* codec_name,
             int (*PCMFrameCallback)(int16_t *, int, int, double *, void *context),
             void* context);

    int Encode(AudioPacketPool* audio_packet_pool);
    void Destroy();
};

}  // namespace trinity

#endif  // TRINITY_AUDIO_ENCODER_H
