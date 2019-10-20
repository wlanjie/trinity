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
//  scale.h
//  opengl
//  缩放效果
//  Created by wlanjie on 2019/9/28.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef scale_h
#define scale_h

#include "frame_buffer.h"
#include "gl.h"

#if __ANDROID__
static const char* SCALE_FRAGMENT_SHADER =
       "#ifdef GL_ES                                                                            \n"
       "precision highp float;                                                                  \n"
       "#endif                                                                                  \n"
       "varying highp vec2 textureCoordinate;                                                   \n"
       "uniform sampler2D inputImageTexture;                                                    \n"
       "uniform highp float scalePercent;                                                       \n"
       "void main() {                                                                           \n"
       "   highp vec2 textureCoordinateToUse = textureCoordinate;                               \n"
       "   highp vec2 center = vec2(0.5, 0.5);                                                  \n"
       "   textureCoordinateToUse -= center;                                                    \n"
       "   textureCoordinateToUse = textureCoordinateToUse / ((scalePercent-1.0)*0.6+1.0);      \n"
       "   textureCoordinateToUse += center;                                                    \n"
       "   lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinateToUse);       \n"
       "   gl_FragColor = textureColor;                                                         \n"
       "}                                                                                       \n";
#else
static const char* SCALE_FRAGMENT_SHADER =
        "varying vec2 textureCoordinate;                                                         \n"
        "uniform sampler2D inputImageTexture;                                                    \n"
        "uniform float scalePercent;                                                             \n"
        "void main() {                                                                           \n"
        "   vec2 textureCoordinateToUse = textureCoordinate;                                     \n"
        "   vec2 center = vec2(0.5, 0.5);                                                        \n"
        "   textureCoordinateToUse -= center;                                                    \n"
        "   textureCoordinateToUse = textureCoordinateToUse / ((scalePercent-1.0)*0.6+1.0);      \n"
        "   textureCoordinateToUse += center;                                                    \n"
        "   vec4 textureColor = texture2D(inputImageTexture, textureCoordinateToUse);            \n"
        "   gl_FragColor = textureColor;                                                         \n"
        "}                                                                                       \n";
#endif
namespace trinity {

class Scale : public FrameBuffer {
 public:
    Scale(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SCALE_FRAGMENT_SHADER) {
        scale_percent_ = nullptr;
        size_ = 0;
        index_ = 0;
    };
    ~Scale() {};
    
    virtual GLuint OnDrawFrame(GLuint texture_id, uint64_t current_time = 0) {
        return FrameBuffer::OnDrawFrame(texture_id, current_time);
    }

    void SetScalePercent(float* scale_percent, int size) {
        scale_percent_ = scale_percent;
        size_ = size;
    }
    
 protected:
   virtual void RunOnDrawTasks() {
        if (size_ > 0 && scale_percent_ != nullptr) {
            index_++;
            SetFloat("scalePercent", scale_percent_[index_ % size_]);
        }
   }
   
 private:
   float* scale_percent_;
   int size_;
   int index_;        
};

} // namespace trinity

#endif /* scale_h */
