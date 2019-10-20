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
//  soul_out.h
//  opengl
//  灵魂出窍特效
//  Created by wlanjie on 2019/10/20.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef soul_out_h
#define soul_out_h

#include "frame_buffer.h"

#if __ANDROID__
static const char* SOUL_SCALE_FRAGMENT_SHADER = 
    "precision highp float;                                                                     \n"
    "varying highp vec2 textureCoordinate;                                                      \n"
    "uniform highp float scalePercent;                                                          \n"
    "uniform lowp float mixturePercent;                                                         \n"
    "uniform float mixturePercent;                                                              \n"
    "uniform sampler2D inputImageTexture;                                                       \n"
    "uniform sampler2D inputImageTexture2;                                                      \n"
    "void main() {                                                                              \n"
    "   lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);               \n"
    "   highp vec2 textureCoordinateToUse = textureCoordinate;                                  \n"
    "   highp vec2 center = vec2(0.5, 0.5);                                                     \n"
    "   textureCoordinateToUse -= center;                                                       \n"
    "   textureCoordinateToUse = textureCoordinateToUse / scalePercent;                         \n"
    "   textureCoordinateToUse += center;                                                       \n"
    "   lowp vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinateToUse);        \n"
    "   gl_FragColor = mix(textureColor, textureColor2, mixturePercent);                        \n"
    "}                                                                                          \n";
#else
static const char* SOUL_SCALE_FRAGMENT_SHADER = 
    "varying vec2 textureCoordinate;                                                            \n"
    "uniform float scalePercent;                                                                \n"
    "uniform float mixturePercent;                                                              \n"
    "uniform sampler2D inputImageTexture;                                                       \n"
    "uniform sampler2D inputImageTexture2;                                                      \n"
    "void main() {                                                                              \n"
    "   vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);                    \n"
    "   vec2 textureCoordinateToUse = textureCoordinate;                                        \n"
    "   vec2 center = vec2(0.5, 0.5);                                                           \n"
    "   textureCoordinateToUse -= center;                                                       \n"
    "   textureCoordinateToUse = textureCoordinateToUse / scalePercent;                         \n"
    "   textureCoordinateToUse += center;                                                       \n"
    "   vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinateToUse);             \n"
    "   gl_FragColor = mix(textureColor, textureColor2, mixturePercent);                        \n"
    "}                                                                                          \n";
#endif

namespace trinity {

class SoulScale : public FrameBuffer {
 public:
    SoulScale(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SOUL_SCALE_FRAGMENT_SHADER) 
        , scale_percent_(nullptr),
          scale_size_(0),
          scale_index_(0), 
          mix_percent_(nullptr),
          mix_size_(0),
          mix_index_(0), 
          mix_texture_id_(0) {
    }
    
    ~SoulScale() = default;
    
    virtual GLuint OnDrawFrame(GLuint texture_id, uint64_t current_time = 0) {
        mix_texture_id_ = texture_id;
        return FrameBuffer::OnDrawFrame(texture_id, current_time);
    }
    
     void SetScalePercent(float* scale_percent, int size) {
         scale_percent_ = scale_percent;
         scale_size_ = size;
     }
     
     void SetMixPercent(float* mix_percent, int size) {
        mix_percent_ = mix_percent;
        mix_size_ = size;
     }
     
 protected:
     virtual void RunOnDrawTasks() {
          if (scale_size_ > 0 && scale_percent_ != nullptr) {
              scale_index_++;
              SetFloat("scalePercent", scale_percent_[scale_index_ % scale_size_]);
          }
          if (mix_size_ > 0 && mix_percent_ != nullptr) {
              mix_index_++;
              SetFloat("mixturePercent", mix_percent_[mix_index_ % mix_size_]);
          }
          if (mix_index_ % 15 == 0) {
              glActiveTexture(GL_TEXTURE1);
              glBindTexture(GL_TEXTURE_2D, mix_texture_id_);
              SetInt("inputImageTexture2", 1);
          }
     }
 private:
    float* scale_percent_;
    int scale_size_;
    int scale_index_;
    
    float* mix_percent_;
    int mix_size_;
    int mix_index_;
    
    int mix_texture_id_;  
};

} // namespace trinity

#endif /* soul_out_h */
