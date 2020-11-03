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

#ifndef TRINITY_VIDEO_ENCODER_ADAPTER_H
#define TRINITY_VIDEO_ENCODER_ADAPTER_H

#include "egl_core.h"
#include "packet_pool.h"

namespace trinity {

typedef enum {
    kStartEncoder = 200,
    kEncodeFrame,
    kEndOfStream,
    kDestroyEncoder,
} EncoderMessage;

class VideoEncoderAdapter {
 public:
    VideoEncoderAdapter();
    virtual ~VideoEncoderAdapter();

    virtual void Init(int width, int height, int video_bit_rate, int frame_rate);

    virtual int CreateEncoder(EGLCore* core) = 0;

    virtual void Encode(int64_t time, int texture_id = 0) = 0;

    virtual void DestroyEncoder() = 0;

    void ResetFpsStartTimeIfNeed(int fps);

 protected:
    int encode_frame_count_;
    int video_width_;
    int video_height_;
    int video_bit_rate_;
    int frame_rate_;
    int64_t fps_change_time_;
    int texture_id_;
    int64_t start_time_;
    const float FLOAT_DELTA = 1e-4;
    PacketPool* packet_pool_;
};

}  // namespace trinity

#endif  // TRINITY_VIDEO_ENCODER_ADAPTER_H
