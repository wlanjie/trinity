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

#ifndef TRINITY_RESAMPLE_H
#define TRINITY_RESAMPLE_H

#include <stdint.h>

extern "C" {
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
};

namespace trinity {

class Resample {
 public:
    Resample();
    virtual ~Resample();

    virtual int Init(int src_rate = 48000, int dst_rate = 44100,
            int max_src_nb_samples = 1024, int src_channels = 1,
            int dst_channels = 1);
    virtual int Process(short* in, uint8_t* out, int src_nb_samples,
            int* out_nb_samples);
    virtual int Process(short** in, uint8_t* out, int src_nb_samples,
            int* out_nb_samples);
    virtual int Convert(uint8_t* out, int src_nb_samples,
            int* out_nb_samples);
    virtual void Destroy();

 private:
    int src_rate_;
    int dst_rate_;
    int src_nb_samples_;
    int64_t src_ch_layout_;
    int64_t dst_ch_layout_;
    int src_nb_channels_;
    int dst_nb_channels_;
    int src_line_size_;
    int dst_line_size_;
    int dst_nb_samples_;
    int max_dst_nb_samples_;
    uint8_t** dst_data_;
    uint8_t** src_data_;
    enum AVSampleFormat dst_sample_fmt_;
    int dst_buffer_size_;
    struct SwrContext* swr_context_;
};

}  // namespace trinity

#endif  // TRINITY_RESAMPLE_H
