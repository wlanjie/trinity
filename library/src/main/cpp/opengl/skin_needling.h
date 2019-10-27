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
//  skin_needling.h
//  opengl
//  毛刺效果
//  Created by wlanjie on 2019/10/24.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef skin_needling_h
#define skin_needling_h

#include "frame_buffer.h"
#include "gl.h"

#ifdef __ANDROID__
static const char* SKIN_NEEDLING_FRAGMENT_SHADER =
    "precision highp float;                                                                         \n"
    "varying highp vec2 textureCoordinate;                                                          \n"
    "uniform sampler2D inputImageTexture;                                                           \n"
    // 横向偏移的值
    "uniform highp float lineJitterX;                                                               \n"
    // 阀值
    "uniform highp float lineJitterY;                                                               \n"
    // 颜色倾斜
    "uniform highp float driftX;                                                                    \n"
    // 整体偏移
    "uniform highp float driftY;                                                                    \n"
    "uniform float uTimeStamp;                                                                      \n"
    "vec2 verticalJump = vec2(0.0, 0.0);                                                            \n"
    "uniform float horizontalShake;                                                                 \n"
    // 随机函数
    "float nrand(float x, float y) {                                                                \n"
    "   return fract(sin(dot(vec2(x, y), vec2(12.9898, 78.233))) * 43758.5453);                     \n"
    "}                                                                                              \n"
    "void main() {                                                                                  \n"
    "   float u = textureCoordinate.x;                                                              \n"
    "   float v = textureCoordinate.y;                                                              \n"
    "   float jitter = nrand(v, uTimeStamp) * 2.0 - 1.0;                                            \n"
    "   jitter *= step(lineJitterY, abs(jitter)) * lineJitterX;                                     \n"
    "   float jump = mix(v, fract(v + verticalJump.y), verticalJump.x);                             \n"
    "   float shake = (nrand(uTimeStamp, 2.0) - 0.5) * horizontalShake;                             \n"
    "   float drift = sin(jump + driftY) * driftX;                                                  \n"
    "   vec4 color1 = texture2D(inputImageTexture, fract(vec2(u + jitter + shake, jump)));          \n"
    "   vec4 color2 = texture2D(inputImageTexture, fract(vec2(u + jitter + shake + drift, jump)));  \n"
    //color1.r, color1.g, color2.b 黄蓝
    //color1.r, color1.g, color1.b 无
    //color1.r, color2.g, color1.b 绿粉红
    //color1.r, color2.g, color2.b 蓝红
    //color2.r, color1.g, color1.b 蓝红
    //color2.r, color1.g, color2.b 粉红绿
    //color2.r, color2.g, color1.b 黄蓝
    //color2.r, color2.g, color2.b 无
    "   gl_FragColor = vec4(color1.r, color2.g, color1.b, 1.0);                                     \n"
    "}                                                                                              \n";
#else
static const char* SKIN_NEEDLING_FRAGMENT_SHADER =
    "varying vec2 textureCoordinate;                                                                \n"
    "uniform sampler2D inputImageTexture;                                                           \n"
    // 横向偏移的值
    "uniform float lineJitterX;                                                                     \n"
    // 阀值
    "uniform float lineJitterY;                                                                     \n"
    // 颜色倾斜
    "uniform float driftX;                                                                          \n"
    // 整体偏移
    "uniform float driftY;                                                                          \n"
    "uniform float uTimeStamp;                                                                      \n"
    "vec2 verticalJump = vec2(0.0, 0.0);                                                            \n"
    "uniform float horizontalShake;                                                                 \n"
    // 随机函数
    "float nrand(float x, float y) {                                                                \n"
    "   return fract(sin(dot(vec2(x, y), vec2(12.9898, 78.233))) * 43758.5453);                     \n"
    "}                                                                                              \n"
    "void main() {                                                                                  \n"
    "   float u = textureCoordinate.x;                                                              \n"
    "   float v = textureCoordinate.y;                                                              \n"
    "   float jitter = nrand(v, uTimeStamp) * 2.0 - 1.0;                                            \n"
    "   jitter *= step(lineJitterY, abs(jitter)) * lineJitterX;                                     \n"
    "   float jump = mix(v, fract(v + verticalJump.y), verticalJump.x);                             \n"
    "   float shake = (nrand(uTimeStamp, 2.0) - 0.5) * horizontalShake;                             \n"
    "   float drift = sin(jump + driftY) * driftX;                                                  \n"
    "   vec4 color1 = texture2D(inputImageTexture, fract(vec2(u + jitter + shake, jump)));          \n"
    "   vec4 color2 = texture2D(inputImageTexture, fract(vec2(u + jitter + shake + drift, jump)));  \n"
    //color1.r, color1.g, color2.b 黄蓝
    //color1.r, color1.g, color1.b 无
    //color1.r, color2.g, color1.b 绿粉红
    //color1.r, color2.g, color2.b 蓝红
    //color2.r, color1.g, color1.b 蓝红
    //color2.r, color1.g, color2.b 粉红绿
    //color2.r, color2.g, color1.b 黄蓝
    //color2.r, color2.g, color2.b 无
    "   gl_FragColor = vec4(color1.r, color2.g, color1.b, 1.0);                                     \n"
    "}                                                                                              \n";

#endif

namespace trinity {

class SkinNeedling : public FrameBuffer {

 public:
    SkinNeedling(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SKIN_NEEDLING_FRAGMENT_SHADER),
                                          line_jitter_x_(nullptr), 
                                          line_jitter_x_size_(0), 
                                          line_jitter_x_index_(0), 
                                          line_jitter_y_(nullptr), 
                                          line_jitter_y_size_(0), 
                                          line_jitter_y_index_(0), 
                                          drift_x_(0),
                                          drift_x_size_(0),
                                          drift_x_index_(0),
                                          drift_y_(nullptr),
                                          drift_y_size_(0),
                                          drift_y_index_(0) {

    }

    ~SkinNeedling() = default;
    
    void setLineJitterX(float* value, int size) {
        line_jitter_x_ = value;
        line_jitter_x_size_ = size;
    }
    
    void setLineJitterY(float* value, int size) {
        line_jitter_y_ = value;
        line_jitter_y_size_ = size;
    }
    
    void setDriftX(float* value, int size) {
        drift_x_ = value;
        drift_x_size_ = size;
    }
    
    void setDriftY(float* value, int size) {
        drift_y_ = value;
        drift_y_size_ = size;
    }
    
 protected:
   virtual void RunOnDrawTasks() {
      if (line_jitter_x_size_ > 0 && line_jitter_x_ != nullptr) {
          line_jitter_x_index_++;
          SetFloat("lineJitterX", line_jitter_x_[line_jitter_x_index_ % line_jitter_x_size_]);
      }
      
      if (line_jitter_y_size_ > 0 && line_jitter_y_ != nullptr) {
          line_jitter_y_index_++;
          SetFloat("lineJitterY", line_jitter_y_[line_jitter_y_index_ % line_jitter_y_size_]);
      }
          
      if (drift_x_size_ > 0 && drift_x_ != nullptr) {
          drift_x_index_++;
          SetFloat("driftX", drift_x_[drift_x_index_ % drift_x_size_]);
      }
              
      if (drift_y_size_ > 0 && drift_y_ != nullptr) {
          drift_y_index_++;
          SetFloat("driftY", drift_y_[drift_y_index_ % drift_y_size_]);
      }
   }
    
 private:
    float* line_jitter_x_;
    int line_jitter_x_size_;
    int line_jitter_x_index_;
    
    float* line_jitter_y_;
    int line_jitter_y_size_;
    int line_jitter_y_index_;
    
    float* drift_x_;
    int drift_x_size_;
    int drift_x_index_;
    
    float* drift_y_;
    int drift_y_size_;
    int drift_y_index_;
};

}  // trinity

#endif /* skin_needling_h */
