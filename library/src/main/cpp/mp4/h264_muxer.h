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
 *
 */

//
// Created by wlanjie on 2019/4/18.
//

#ifndef TRINITY_H264_MUXER_H
#define TRINITY_H264_MUXER_H

#include "mp4_muxer.h"

namespace trinity {

class H264Muxer : public Mp4Muxer {
 public:
    H264Muxer();
    virtual ~H264Muxer();
    virtual int Stop();

 protected:
    int last_presentation_time_ms_;
    virtual int WriteVideoFrame(AVFormatContext* oc, AVStream* st);
    virtual double GetVideoStreamTimeInSecs();
    // TODO cpplint检查有问题
    uint32_t FindStartCode(uint8_t* in_buffer, uint32_t in_ui32_buffer_size,
            uint32_t in_ui32_code, uint32_t &out_ui32_processed_bytes);
    void ParseH264SequenceHeader(uint8_t* in_buffer, uint32_t in_ui32_size,
            uint8_t** in_sps_buffer, int& in_sps_size,
            uint8_t** in_pps_buffer, int& in_pps_size);
};

}  // namespace trinity

#endif  // TRINITY_H264_MUXER_H
