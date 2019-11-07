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
//  shake.h
//  opengl
//  抖动效果
//  Created by wlanjie on 2019/10/25.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef shake_h
#define shake_h

#include "frame_buffer.h"
#include "gl.h"

#ifdef __ANDROID__
static const char* SHAKE_FRAGMENT_SHADER =
       "precision highp float;                                                                                                                             \n"  
       "varying highp vec2 textureCoordinate;                                                                                                              \n"
       "uniform sampler2D inputImageTexture;                                                                                                               \n"
       "uniform float scale;                                                                                                                               \n"
       "void main() {                                                                                                                                      \n"
       "   vec2 newTextureCoordinate = vec2((scale - 1.0) * 0.5 + textureCoordinate.x / scale, (scale - 1.0) * 0.5 + textureCoordinate.y / scale);         \n"
       "   vec4 textureColor = texture2D(inputImageTexture, newTextureCoordinate);                                                                         \n"
       "   vec4 shiftColor1 = texture2D(inputImageTexture, newTextureCoordinate + vec2(-0.05 * (scale - 1.0), -0.05 * (scale - 1.0)));                     \n"
       "   vec4 shiftColor2 = texture2D(inputImageTexture, newTextureCoordinate + vec2(-0.1 * (scale - 1.0), -0.1 * (scale - 1.0)));                       \n"
       "   vec3 blendFirstColor = vec3(textureColor.r, textureColor.g, textureColor.b);                                                                    \n"
       "   vec3 blend3DColor = vec3(shiftColor2.r, blendFirstColor.g, blendFirstColor.b);                                                                  \n"
       "   gl_FragColor = vec4(shiftColor2.r, textureColor.g, shiftColor1.b, textureColor.a);                                                              \n"
       "}                                                                                                                                                  \n";
#else
static const char* SHAKE_FRAGMENT_SHADER =
        "varying vec2 textureCoordinate;                                                                                                                    \n"
        "uniform sampler2D inputImageTexture;                                                                                                               \n"
        "uniform float scale;                                                                                                                               \n"
        "void main() {                                                                                                                                      \n"
        "   vec2 newTextureCoordinate = vec2((scale - 1.0) * 0.5 + textureCoordinate.x / scale, (scale - 1.0) * 0.5 + textureCoordinate.y / scale);         \n"
        "   vec4 textureColor = texture2D(inputImageTexture, newTextureCoordinate);                                                                         \n"
        "   vec4 shiftColor1 = texture2D(inputImageTexture, newTextureCoordinate + vec2(-0.05 * (scale - 1.0), -0.05 * (scale - 1.0)));                     \n"
        "   vec4 shiftColor2 = texture2D(inputImageTexture, newTextureCoordinate + vec2(-0.1 * (scale - 1.0), -0.1 * (scale - 1.0)));                       \n"
        "   vec3 blendFirstColor = vec3(textureColor.r, textureColor.g, textureColor.b);                                                                    \n"
        "   vec3 blend3DColor = vec3(shiftColor2.r, blendFirstColor.g, blendFirstColor.b);                                                                  \n"
        "   gl_FragColor = vec4(shiftColor2.r, textureColor.g, shiftColor1.b, textureColor.a);                                                              \n"
        "}                                                                                                                                                  \n";
#endif

namespace trinity {

class Shake : public FrameBuffer {

 public:
    Shake(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SHAKE_FRAGMENT_SHADER),
        scale_percent_(nullptr),
        size_(0),
        index_(0) {

    }

    ~Shake() {
        if (nullptr != scale_percent_) {
            delete scale_percent_;
            scale_percent_ = nullptr;
        }
    }
    
    void SetScalePercent(float* scale_percent, int size) {
        scale_percent_ = scale_percent;
        size_ = size;
    }
 protected:
    virtual void RunOnDrawTasks() {
       if (size_ > 0 && scale_percent_ != nullptr) {
           SetFloat("scale", scale_percent_[index_ % size_]);
           index_++;
       }
    }
  
 private:
    float* scale_percent_;
    int size_;
    int index_;  
};

}  // trinity

#endif /* shake_h */
