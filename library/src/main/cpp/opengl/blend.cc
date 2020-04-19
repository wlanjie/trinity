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
 */

//
//  blend.cc
//  opengl
//
//  Created by wlanjie on 2019/12/28.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#include "blend.h"
#include <cstdio>
#include <cstdlib>

#if __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "android_xlog.h"
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__) // NOLINT
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#endif

namespace trinity {

Blend::Blend(const char* fragment_shader) :
    frame_buffer_texture_id_(0)
    , frame_buffer_id_(0)
    , frame_buffer_(nullptr)
    , source_width_(0)
    , source_height_(0) {
    default_vertex_coordinates_ = new GLfloat[8];
    default_texture_coordinates_ = new GLfloat[8];
    default_vertex_coordinates_[0] = -1.0F;
    default_vertex_coordinates_[1] = -1.0F;
    default_vertex_coordinates_[2] = 1.0F;
    default_vertex_coordinates_[3] = -1.0F;
    default_vertex_coordinates_[4] = -1.0F;
    default_vertex_coordinates_[5] = 1.0F;
    default_vertex_coordinates_[6] = 1.0F;
    default_vertex_coordinates_[7] = 1.0F;

    default_texture_coordinates_[0] = 0.0F;
    default_texture_coordinates_[1] = 0.0F;
    default_texture_coordinates_[2] = 1.0F;
    default_texture_coordinates_[3] = 0.0F;
    default_texture_coordinates_[4] = 0.0F;
    default_texture_coordinates_[5] = 1.0F;
    default_texture_coordinates_[6] = 1.0F;
    default_texture_coordinates_[7] = 1.0F;

    program_ = CreateProgram(BLEND_VERTEX_SHADER, fragment_shader);
    second_program_ = CreateProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    glUseProgram(second_program_);
}

Blend::~Blend() {
    if (nullptr != default_vertex_coordinates_) {
        delete[] default_vertex_coordinates_;
        default_vertex_coordinates_ = nullptr;
    }
    if (nullptr != default_texture_coordinates_) {
        delete[] default_texture_coordinates_;
        default_texture_coordinates_ = nullptr;
    }
    DeleteFrameBuffer();
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (second_program_ != 0) {
        glDeleteProgram(second_program_);
        second_program_ = 0;
    }
}

void Blend::CreateFrameBuffer(int width, int height) {
    DeleteFrameBuffer();
    glGenTextures(1, &frame_buffer_texture_id_);
    glGenFramebuffers(1, &frame_buffer_id_);
    glBindTexture(GL_TEXTURE_2D, frame_buffer_texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame_buffer_texture_id_, 0);

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
#if __ANDROID__
        LOGE("frame buffer error");
#endif
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    frame_buffer_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
}

void Blend::DeleteFrameBuffer() {
    if (frame_buffer_texture_id_ != 0) {
        glDeleteTextures(1, &frame_buffer_texture_id_);
        frame_buffer_texture_id_ = 0;
    }
    if (frame_buffer_id_ != 0) {
        glDeleteFramebuffers(1, &frame_buffer_id_);
        frame_buffer_id_ = 0;
    }
    if (nullptr != frame_buffer_) {
        delete frame_buffer_;
        frame_buffer_ = nullptr;
    }
}

int Blend::OnDrawFrame(int texture_id, int sticker_texture_id, int width, int height, GLfloat* matrix, float alpha_factor) {
    if (source_width_ != width && source_height_ != height) {
        source_width_ = width;
        source_height_ = height;
        DeleteFrameBuffer();
        CreateFrameBuffer(width, height);
    }
    int frame_buffer_texture_id =  frame_buffer_->OnDrawFrame(texture_id);
    
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    glViewport(0, 0, width, height);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(second_program_);
//    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, frame_buffer_texture_id);
    auto blend_input_image_texture_location = glGetUniformLocation(second_program_, "inputImageTexture");
    glUniform1i(blend_input_image_texture_location, 2);
    auto position2_location = glGetAttribLocation(second_program_, "position");
    glEnableVertexAttribArray(position2_location);
    glVertexAttribPointer(position2_location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), default_vertex_coordinates_);
    auto input_texture_coordinate2_locaiton = glGetAttribLocation(second_program_, "inputTextureCoordinate");
    glEnableVertexAttribArray(input_texture_coordinate2_locaiton);
    glVertexAttribPointer(input_texture_coordinate2_locaiton, 2, GL_FLOAT, GL_FALSE,
            2 * sizeof(GLfloat), default_texture_coordinates_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(position2_location);
    glDisableVertexAttribArray(input_texture_coordinate2_locaiton);
    
    glUseProgram(program_);
    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, frame_buffer_texture_id);
    auto input_image_texture_location = glGetUniformLocation(program_, "inputImageTexture");
    glUniform1i(input_image_texture_location, 2);
    glActiveTexture(GL_TEXTURE3);
    // bind 贴纸纹理
    glBindTexture(GL_TEXTURE_2D, sticker_texture_id);
    auto input_image_texture2_location = glGetUniformLocation(program_, "inputImageTexture2");
    glUniform1i(input_image_texture2_location, 3);
    auto alpha_factor_location = glGetUniformLocation(program_, "alphaFactor");
    glUniform1f(alpha_factor_location, alpha_factor);
    auto matrix_location = glGetUniformLocation(program_, "matrix");
    
    // 设置矩阵
    if (nullptr == matrix) {
        GLfloat m[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        glUniformMatrix4fv(matrix_location, 1, GL_FALSE, m);
    } else {
        glUniformMatrix4fv(matrix_location, 1, GL_FALSE, matrix);
    }

    auto position_location = glGetAttribLocation(program_, "position");
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), default_vertex_coordinates_);
    auto input_texture_coordinate_location = glGetAttribLocation(program_, "inputTextureCoordinate");
    glEnableVertexAttribArray(input_texture_coordinate_location);
    glVertexAttribPointer(input_texture_coordinate_location, 2, GL_FLOAT, GL_FALSE,
            2 * sizeof(GLfloat), default_texture_coordinates_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
//    char* buffer = reinterpret_cast<char*>(malloc(width * height * 4));
//    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
//    stbi_write_png("/Users/wlanjie/Desktop/offscreen.png",
//                   width, height, 4,
//                   buffer + (width * 4 * (height - 1)),
//                   -width * 4);
//    free(buffer);
    
    glDisableVertexAttribArray(position_location);
    glDisableVertexAttribArray(input_texture_coordinate_location);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return frame_buffer_texture_id_;
}

int Blend::CreateProgram(const char *vertex, const char *fragment) {
    int program = glCreateProgram();
    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    CompileShader(vertex, vertexShader);
    CompileShader(fragment, fragmentShader);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    int ret = Link(program);
    if (ret != 0) {
        program = 0;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void Blend::CompileShader(const char *shader_string, GLuint shader) {
    glShaderSource(shader, 1, &shader_string, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    printf("%s\n", shader_string);
    printf("==================\n");
    if (!compiled) {
        GLint infoLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
            if (buf) {
                LOGE("shader_string: %s", shader_string);
                glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                LOGE("Could not compile %d:\n%s\n", shader, buf);
                free(buf);
            }
            glDeleteShader(shader);
        }
    }
}

int Blend::Link(int program) {
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint infoLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
            if (buf) {
                glGetProgramInfoLog(program, infoLen, nullptr, buf);
                printf("%s", buf);
                free(buf);
            }
            glDeleteProgram(program);
            return -1;
        }
    }
    return 0;
}

}  // namespace trinity
