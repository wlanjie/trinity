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
// Created by wlanjie on 2019-12-18.
//

#ifndef TRINITY_IMAGE_BUFFER_H
#define TRINITY_IMAGE_BUFFER_H

#if __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#endif

class ImageBuffer {
 public:
    ImageBuffer(int width, int height, unsigned char* image_buffer) {
        glGenTextures(1, &texture_id_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // 这里如果GL_REPEAT人脸道具会有好多重复显示
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffer);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~ImageBuffer() {
        glDeleteTextures(1, &texture_id_);
    }

    GLuint GetTextureId() {
        return texture_id_;
    }

 private:
    GLuint texture_id_;
};

#endif // TRINITY_IMAGE_BUFFER_H
