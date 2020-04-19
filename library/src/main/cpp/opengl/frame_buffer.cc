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

#include "frame_buffer.h"
#if __ANDROID__
#include "android_xlog.h"
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define LOGI // NOLINT
#endif

namespace trinity {

FrameBuffer::FrameBuffer(int width, int height, const char *vertex, const char *fragment)
    : OpenGL(width, height, vertex, fragment)
    , texture_id_(0)
    , frameBuffer_id_(0)
    , vertex_coordinate_(nullptr)
    , texture_coordinate_(nullptr)
    , start_time_(0)
    , end_time_(INT32_MAX) {
    CompileFrameBuffer(width, height);
}

void FrameBuffer::CompileFrameBuffer(int camera_width, int camera_height) {
    glGenTextures(1, &texture_id_);
    glGenFramebuffers(1, &frameBuffer_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, camera_width, camera_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
#if __ANDROID__
        LOGE("frame buffer error");
#endif
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];
    vertex_coordinate_[0] = -1.0F;
    vertex_coordinate_[1] = -1.0F;
    vertex_coordinate_[2] = 1.0F;
    vertex_coordinate_[3] = -1.0F;
    vertex_coordinate_[4] = -1.0F;
    vertex_coordinate_[5] = 1.0F;
    vertex_coordinate_[6] = 1.0F;
    vertex_coordinate_[7] = 1.0F;

    texture_coordinate_[0] = 0.0F;
    texture_coordinate_[1] = 0.0F;
    texture_coordinate_[2] = 1.0F;
    texture_coordinate_[3] = 0.0F;
    texture_coordinate_[4] = 0.0F;
    texture_coordinate_[5] = 1.0F;
    texture_coordinate_[6] = 1.0F;
    texture_coordinate_[7] = 1.0F;
}


FrameBuffer::~FrameBuffer() {
    glDeleteTextures(1, &texture_id_);
    glDeleteFramebuffers(1, &frameBuffer_id_);
    delete[] vertex_coordinate_;
    delete[] texture_coordinate_;
}

void FrameBuffer::SetStartTime(int time) {
    start_time_ = time;
}

void FrameBuffer::SetEndTime(int time) {
    end_time_ = time;
}

GLuint FrameBuffer::OnDrawFrame(GLuint texture_id, uint64_t current_time) {
    if (current_time >= start_time_ && current_time <= end_time_) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
        ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return texture_id_;
    }
    return texture_id;
}

GLuint FrameBuffer::OnDrawFrame(GLuint texture_id, GLfloat *matrix, uint64_t current_time) {
    if (current_time >= start_time_ && current_time <= end_time_) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
        ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_, matrix);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return texture_id_;
    }
    return texture_id;
}

GLuint
FrameBuffer::OnDrawFrame(GLuint texture_id, const GLfloat *vertex_coordinate, const GLfloat *texture_coordinate, uint64_t current_time) {
    if (current_time >= start_time_ && current_time <= end_time_) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
        ProcessImage(texture_id, vertex_coordinate, texture_coordinate);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return texture_id_;
    }
    return texture_id;
}

GLuint
FrameBuffer::OnDrawFrame(GLuint texture_id, const GLfloat *vertex_coordinate, const GLfloat *texture_coordinate,
        GLfloat* matrix, uint64_t current_time) {
    if (current_time >= start_time_ && current_time <= end_time_) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
        ProcessImage(texture_id, vertex_coordinate, texture_coordinate, matrix);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return texture_id_;
    }
    return texture_id;
}

}  // namespace trinity
