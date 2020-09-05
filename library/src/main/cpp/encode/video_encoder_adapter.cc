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
// Created by wlanjie on 2019/4/15.
//

#include <math.h>
#include "video_encoder_adapter.h"
#include "tools.h"

namespace trinity {

VideoEncoderAdapter::VideoEncoderAdapter()
    : encode_frame_count_(0),
      video_width_(0),
      video_height_(0),
      video_bit_rate_(0),
      frame_rate_(0),
      fps_change_time_(0),
      texture_id_(0),
      start_time_(0),
      packet_pool_(nullptr) {
}

VideoEncoderAdapter::~VideoEncoderAdapter() {}

void VideoEncoderAdapter::Init(int width, int height, int video_bit_rate, int frame_rate) {
    packet_pool_ = PacketPool::GetInstance();
    video_width_ = width;
    video_height_ = height;
    video_bit_rate_ = video_bit_rate;
    frame_rate_ = frame_rate;
    encode_frame_count_ = 0;
}

void VideoEncoderAdapter::ResetFpsStartTimeIfNeed(int fps) {
    if (fabs(fps - frame_rate_) > FLOAT_DELTA) {
        frame_rate_ = fps;
        encode_frame_count_ = 0;
        fps_change_time_ = getCurrentTime();
    }
}

}  // namespace trinity
