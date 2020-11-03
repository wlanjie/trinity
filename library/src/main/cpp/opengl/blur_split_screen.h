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
//  blur_split_screen.h
//  opengl
//  模糊分屏
//  Created by wlanjie on 2019/8/31.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef blur_split_screen_h
#define blur_split_screen_h

#include "frame_buffer.h"
#include "gaussian_blur.h"
#ifdef __ANDROID__
#include "android_xlog.h"
#endif

namespace trinity {

static const char* SAMPLE_VERTEX_SHADER =
    "attribute vec4 position;\n"
    "attribute vec2 inputTextureCoordinate;\n"
    "varying vec2 textureCoordinate;\n"
    "void main() {\n"
    "   gl_Position = position;\n"
    "   textureCoordinate = (inputTextureCoordinate.xy - 0.5) / 1.5 + 0.5;\n"
    "}\n";
    
static const char* BLEND_FRAGMENT_SHADER =
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif\n"
    "varying vec2 textureCoordinate;\n"
    "uniform sampler2D inputImageTexture;\n"
    "uniform sampler2D inputImageTextureBlurred;\n"
    "void main() {\n"
    "   int col = int(textureCoordinate.y * 3.0);\n"
    "   vec2 textureCoordinateToUse = textureCoordinate;\n"
    "   textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 3.0) * 3.0;\n"
    "   textureCoordinateToUse.y = textureCoordinateToUse.y / 3.0 + 1.0 / 3.0;\n"
    "   if (col == 1) {\n"
    "       gl_FragColor = texture2D(inputImageTexture, textureCoordinateToUse);\n"
    "   } else {\n"
    "       gl_FragColor = texture2D(inputImageTextureBlurred, textureCoordinate);\n"
    "   }\n"
    "}\n";
  
class BlurSplitScreen : public FrameBuffer {
 public:
    BlurSplitScreen(int width, int height) : FrameBuffer(width, height, SAMPLE_VERTEX_SHADER, BLEND_FRAGMENT_SHADER) {
        blur_texture_id_ = 0;
        gaussian_blur_ = new GaussianBlur(width, height, 1.0f);
    }
    
    ~BlurSplitScreen() {
        delete gaussian_blur_;
        gaussian_blur_ = nullptr;
    }
    
    virtual GLuint OnDrawFrame(GLuint texture_id, uint64_t current_time = 0) {
        blur_texture_id_ = gaussian_blur_->OnDrawFrame(texture_id);
        return FrameBuffer::OnDrawFrame(texture_id, current_time);
    }
    
 protected:
    virtual void RunOnDrawTasks() {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blur_texture_id_);
        SetInt("inputImageTextureBlurred", 1);
    }
 private:
    GaussianBlur* gaussian_blur_;
    int blur_texture_id_;
};
  
}  // namespace trinity

#endif /* blur_split_screen_h */
