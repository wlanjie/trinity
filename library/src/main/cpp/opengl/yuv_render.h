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
// Created by wlanjie on 2019/4/10.
//

#ifndef TRINITY_YUV_RENDER_H
#define TRINITY_YUV_RENDER_H

#if __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "egl_core.h"
#elif __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#endif

extern "C" {
#include "libavcodec/avcodec.h"
};

#define VERTEX_COORDINATE_COUNT	8

namespace trinity {

class YuvRender {
 public:
    YuvRender();
    ~YuvRender();
    virtual GLuint DrawFrame(AVFrame* frame, const GLfloat* matrix,
            const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate);

 private:
    int Initialize(int width, int height, const char* fragment_shader);
    void Destroy();
    void CopyFrame(uint8_t *dst, uint8_t *src, int width, int height, int line_size);
    void CreateProgram(const char* vertex, const char* fragment);
    void CompileShader(const char* shader_string, GLuint shader);
    void Link();

 private:
    GLuint frame_buffer_id_;
    GLuint texture_id_;
    GLuint textures[3];
    GLuint program_;
    GLuint vertex_coordinate_location_;
    GLuint texture_coordinate_location_;
    GLuint matrix_location_;
    GLint uniform_samplers_[3];

    uint8_t *y;
    uint8_t *u;
    uint8_t *v;
    int y_size_;
    int u_size_;
    int v_size_;
};

}  // namespace trinity

#endif  // TRINITY_YUV_RENDER_H
