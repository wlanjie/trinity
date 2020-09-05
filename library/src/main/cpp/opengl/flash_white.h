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

#if __ANDROID__
#include "android_xlog.h"
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define LOGI 
#endif

// 闪白效果
static const char* FLASH_WHITE_FRAGMENT_SHADER =
        "precision highp float;\n"
        "varying vec2 textureCoordinate;\n"
        "uniform sampler2D inputImageTexture;\n"
        "uniform float alphaTimeLine;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(inputImageTexture, textureCoordinate) + alphaTimeLine * vec4(1.0,1.0,1.0,0.0);\n"
        "}\n";

namespace trinity {

class FlashWhite : public FrameBuffer {
 public:
    FlashWhite(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, FLASH_WHITE_FRAGMENT_SHADER),
            alpha_time_(nullptr),
            size_(0),
            index_(0) {
        SetOutput(width, height);
    }

    ~FlashWhite() {
        if (nullptr != alpha_time_) {
            delete[] alpha_time_;
            alpha_time_ = 0;
        }
        size_ = 0;
        index_ = 0;
    }

    void SetAlphaTime(float* alpha_time, int size) {
        if (alpha_time_ != nullptr) {
            delete[] alpha_time_;
            alpha_time_ = nullptr;
        }
        size_ = size;
        alpha_time_ = new float[size];
        memcpy(alpha_time_, alpha_time, size * sizeof(float));
    }

 protected:
    virtual void RunOnDrawTasks() {
        FrameBuffer::RunOnDrawTasks();
        if (size_ > 0 && nullptr != alpha_time_) {
            SetFloat("alphaTimeLine", alpha_time_[index_ % size_]);
            index_++;
        }
    }

 private:
    float* alpha_time_;
    int size_;
    int index_;
};

}  // namespace trinity

#endif  // TRINITY_FLASH_WHITE_H
