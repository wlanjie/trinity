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
#include "gl.h"

#ifdef __ANDROID__
static const char* SOUL_SCALE_FRAGMENT_SHADER = 
    "precision highp float;                                                                     \n"
    "varying highp vec2 textureCoordinate;                                                      \n"
    // 放大系数
    "uniform highp float scalePercent;                                                          \n"
    // 两个texture合成时的透明系数
    "uniform lowp float mixturePercent;                                                         \n"
    "uniform sampler2D inputImageTexture;                                                       \n"
    "uniform sampler2D inputImageTexture2;                                                      \n"
    "void main() {                                                                              \n"
    "   lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);               \n"
    "   highp vec2 textureCoordinateToUse = textureCoordinate;                                  \n"
    //  纹理的中心点, 从中心点开始放大
    "   highp vec2 center = vec2(0.5, 0.5);                                                     \n"
    "   textureCoordinateToUse -= center;                                                       \n"
    //  根据设置进来的缩放系数进行纹理的坐标缩放
    "   textureCoordinateToUse = textureCoordinateToUse / scalePercent;                         \n"
    "   textureCoordinateToUse += center;                                                       \n"
    //  运算后的坐标如 (0.2, 0.8)
    //  获取放大后的颜色信息
    "   lowp vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinateToUse);        \n"
    //  原始纹理和放大后的纹理做合成
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
    
    ~SoulScale() {
        if (nullptr != scale_percent_) {
            delete[] scale_percent_;
            scale_percent_ = nullptr;
        }
        scale_size_ = 0;
        if (nullptr != mix_percent_) {
            delete[] mix_percent_;
            mix_percent_ = nullptr;
        }
        mix_size_ = 0;
    }
    
    virtual GLuint OnDrawFrame(GLuint texture_id, uint64_t current_time = 0) {
        mix_texture_id_ = texture_id;
        return FrameBuffer::OnDrawFrame(texture_id, current_time);
    }
    
     void SetScalePercent(float* scale_percent, int size) {
         if (nullptr != scale_percent_) {
             delete[] scale_percent_;
             scale_percent_ = nullptr;
         }
         scale_percent_ = new float[size];
         memcpy(scale_percent_, scale_percent, size * sizeof(float));
         scale_size_ = size;
     }
     
     void SetMixPercent(float* mix_percent, int size) {
        if (nullptr != mix_percent_) {
            delete[] mix_percent_;
            mix_percent_ = nullptr;
        }
        mix_percent_ = new float[size];
        memcpy(mix_percent_, mix_percent, size * sizeof(float));
        mix_size_ = size;
     }
     
 protected:
     virtual void RunOnDrawTasks() {
          if (scale_size_ > 0 && scale_percent_ != nullptr) {
              SetFloat("scalePercent", scale_percent_[scale_index_ % scale_size_]);
              scale_index_++;
          }
          if (mix_size_ > 0 && mix_percent_ != nullptr) {
              SetFloat("mixturePercent", mix_percent_[mix_index_ % mix_size_]);
              mix_index_++;
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
