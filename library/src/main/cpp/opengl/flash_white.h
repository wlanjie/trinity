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
// Created by wlanjie on 2019-06-03.
//

#ifndef TRINITY_FLASH_WHITE_H
#define TRINITY_FLASH_WHITE_H

#include "frame_buffer.h"
#include "gl.h"

#define MAX_FRAMES 16
#define SKIP_FRAMES 4

namespace trinity {

class FlashWhite : public FrameBuffer {
 public:
    FlashWhite(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, FLASH_WHITE_FRAGMENT_SHADER) {
        progress_ = 0;
        frames_ = 0;
        SetOutput(width, height);
    }

 protected:
    virtual void RunOnDrawTasks() {
        FrameBuffer::RunOnDrawTasks();
        if (frames_ <= MAX_FRAMES) {
            progress_ = frames_ * 1.0f / MAX_FRAMES;
        } else {
            progress_ = 2.0f - frames_ * 1.0f / MAX_FRAMES;
        }
//        progress_ = frames_ * 1.0f / MAX_FRAMES;
        if (progress_ > 1) {
            progress_ = 0;
        }
        frames_++;
        if (frames_ > MAX_FRAMES) {
            frames_ = 0;
        }
        SetFloat("exposureColor", progress_);
    }

 private:
    int frames_;
    float progress_;
};

}  // namespace trinity

#endif  // TRINITY_FLASH_WHITE_H
