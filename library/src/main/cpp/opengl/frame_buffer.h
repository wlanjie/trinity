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
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_FRAME_BUFFER_H
#define TRINITY_FRAME_BUFFER_H

#include "opengl.h"

namespace trinity {

class FrameBuffer : public OpenGL {
 public:
    FrameBuffer(int width, int height, const char* vertex, const char* fragment);
    void CompileFrameBuffer(int camera_width, int camera_height);
    ~FrameBuffer();

    void SetStartTime(int time);
    void SetEndTime(int time);
//    void InitMessageQueue(const char* vertex, const char* fragment);
    virtual GLuint OnDrawFrame(GLuint texture_id, uint64_t current_time = 0);
    virtual GLuint OnDrawFrame(GLuint texture_id, GLfloat* matrix, uint64_t current_time = 0);
    virtual GLuint OnDrawFrame(GLuint texture_id, const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate, uint64_t current_time = 0);
    virtual GLuint OnDrawFrame(GLuint texture_id, const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate,
            GLfloat* matrix, uint64_t current_time = 0);

    GLuint GetTextureId() {
        return texture_id_;
    }

    GLuint GetFrameBufferId() {
        return frameBuffer_id_;
    }

 private:
    GLuint texture_id_;
    GLuint frameBuffer_id_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    int start_time_;
    int end_time_;
};

}  // namespace trinity

#endif  // TRINITY_FRAME_BUFFER_H
