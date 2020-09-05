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
// Created by wlanjie on 2019/4/19.
//

#ifndef TRINITY_RECORD_PROCESSOR_H
#define TRINITY_RECORD_PROCESSOR_H

#include <stdint.h>
#include "packet_pool.h"
#include "audio_encoder_adapter.h"

extern "C" {
#include "sonic.h"
}

namespace trinity {

class RecordProcessor {
 public:
    RecordProcessor();
    ~RecordProcessor();

    void InitAudioBufferSize(int sample_rate, int audio_buffer_size, float speed);
    int PushAudioBufferToQueue(short* samples, int size);
    void FlushAudioBufferToQueue();
    void Destroy();

 private:
    bool DetectNeedCorrect(int64_t data_present_time_mills, int64_t recording_time_mills, int *correct_time_mills);
    void CopyToAudioSamples(short* buffer, int length);
    int CorrectRecordBuffer(int correct_time_mills);
    AudioPacket* GetSilentDataPacket(int audioBufferSize);

 private:
    int audio_sample_rate_;
    short* audio_samples_;
    float audio_sample_time_mills_;
    int audio_sample_cursor_;
    int audio_buffer_size_;
    int audio_buffer_time_mills_;
    PacketPool* packet_pool_;
    bool recording_flag_;
    int64_t start_time_mills_;
    int64_t data_accumulate_time_mills_;
    AudioEncoderAdapter* audio_encoder_;
    sonicStream sonic_stream_;
};

}  // namespace trinity

#endif  // TRINITY_RECORD_PROCESSOR_H
