/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
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
 */

//
// Created by wlanjie on 2020/7/2.
//

#ifndef TRINITY_APP_BACKGROUND_H
#define TRINITY_APP_BACKGROUND_H

#include "opengl.h"

namespace trinity {

class Background {
 public:
    explicit Background();
    virtual ~Background();
    void SetColor(int red, int green, int blue, int alpha);
    void SetImage(unsigned char* buffer, int width, int height);
    int OnDrawFrame(GLuint texture_id, int width, int height);
    int OnDrawFrame(GLuint texture_id, int width, int height, float* vertex_coordinate);
 private:
    void CreateFrameBuffer(int width, int height);
    void DeleteFrameBuffer();
    int CreateProgram(const char* vertex, const char* fragment);
    void CompileShader(const char* shader_string, GLuint shader);
    int Link(int program);

 private:
    int program_;
    int second_program_;
    GLuint frame_buffer_id_;
    GLuint frame_buffer_texture_id_;
    int source_width_;
    int source_height_;
    int start_time_;
    int end_time_;
    int red_;
    int green_;
    int blue_;
    int alpha_;
    GLuint background_image_texture_;
};

}

#endif  // TRINITY_APP_BACKGROUND_H
