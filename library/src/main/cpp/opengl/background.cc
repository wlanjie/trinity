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

#include "background.h"
#include <cstdio>
#include <cstdlib>
#include "gl.h"

//#define STB_IMAGE_WRITE_STATIC
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_IMAGE_RESIZE_STATIC
//#define STB_IMAGE_RESIZE_IMPLEMENTATION
//#include "stb_image_write.h"
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//#include "stb_image_resize.h"

#ifdef __ANDROID__
#include "android_xlog.h"
#else
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__)
#define LOGI(format, ...) fprintf(stdout, format, __VA_ARGS__)
#endif

static const char* BACKGROUND_FRAGMENT =
        "#ifdef GL_ES                                                                           \n"
        "precision highp float;                                                                 \n"
        "#endif                                                                                 \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "uniform vec4 color;                                                                    \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "void main() {                                                                          \n"
        "    vec4 texture = texture2D(inputImageTexture, textureCoordinate);                    \n"
        "    gl_FragColor = texture + color;                                                    \n"
        "}                                                                                      \n";

namespace trinity {

Background::Background() :
    frame_buffer_texture_id_(0)
    , frame_buffer_id_(0)
    , source_width_(0)
    , source_height_(0)
    , start_time_(0)
    , end_time_(INT32_MAX)
    , red_(0)
    , green_(0)
    , blue_(0)
    , alpha_(0)
    , background_image_texture_(0) {
    program_ = CreateProgram(DEFAULT_VERTEX_SHADER, BACKGROUND_FRAGMENT);
    second_program_ = CreateProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    glUseProgram(second_program_);
}

Background::~Background() {
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

void Background::CreateFrameBuffer(int width, int height) {
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
}

void Background::DeleteFrameBuffer() {
    if (frame_buffer_texture_id_ != 0) {
        glDeleteTextures(1, &frame_buffer_texture_id_);
        frame_buffer_texture_id_ = 0;
    }
    if (frame_buffer_id_ != 0) {
        glDeleteFramebuffers(1, &frame_buffer_id_);
        frame_buffer_id_ = 0;
    }
}

void Background::SetColor(int red, int green, int blue, int alpha) {
    red_ = red;
    green_ = green;
    blue_ = blue;
    alpha_ = alpha;
}

void Background::SetImage(unsigned char* buffer, int width, int height) {
    if (background_image_texture_ != 0) {
        glDeleteTextures(1, &background_image_texture_);
    }
    glGenTextures(1, &background_image_texture_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, background_image_texture_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Background::OnDrawFrame(GLuint texture_id, int width, int height) {
//    return OnDrawFrame(texture_id, width, height, DEFAULT_VERTEX_COORDINATE);
    return 0;
}

int Background::OnDrawFrame(GLuint texture_id, int width, int height, float *vertex_coordinate) {
    if (source_width_ != width || source_height_ != height) {
        source_width_ = width;
        source_height_ = height;
        if (frame_buffer_id_ == 0) {
            CreateFrameBuffer(width, height);
        } else {
            glBindTexture(GL_TEXTURE_2D, frame_buffer_texture_id_);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

    }
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    glViewport(0, 0, width, height);
    glClearColor(1.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 画背景
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, background_image_texture_);
    auto input_image_texture_location = glGetUniformLocation(program_, "inputImageTexture");
    glUniform1i(input_image_texture_location, 0);
    auto color_location = glGetUniformLocation(program_, "color");
    glUniform4f(color_location, red_ * 1.0f / 255, green_ * 1.0f / 255, blue_ * 1.0f / 255, alpha_ * 1.0f / 255);
    glActiveTexture(GL_TEXTURE3);
    auto position_location = glGetAttribLocation(program_, "position");
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), DEFAULT_VERTEX_COORDINATE);
    auto input_texture_coordinate_location = glGetAttribLocation(program_, "inputTextureCoordinate");
    glEnableVertexAttribArray(input_texture_coordinate_location);
    glVertexAttribPointer(input_texture_coordinate_location, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(GLfloat), DEFAULT_TEXTURE_COORDINATE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(position_location);
    glDisableVertexAttribArray(input_texture_coordinate_location);

    // 画视频内容
    glUseProgram(second_program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    auto blend_input_image_texture_location = glGetUniformLocation(second_program_, "inputImageTexture");
    glUniform1i(blend_input_image_texture_location, 0);
    auto position2_location = glGetAttribLocation(second_program_, "position");
    glEnableVertexAttribArray(position2_location);
    glVertexAttribPointer(position2_location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertex_coordinate);
    auto input_texture_coordinate2_location = glGetAttribLocation(second_program_, "inputTextureCoordinate");
    glEnableVertexAttribArray(input_texture_coordinate2_location);
    glVertexAttribPointer(input_texture_coordinate2_location, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(GLfloat), DEFAULT_TEXTURE_COORDINATE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(position2_location);
    glDisableVertexAttribArray(input_texture_coordinate2_location);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return  frame_buffer_texture_id_;
}

int Background::CreateProgram(const char *vertex, const char *fragment) {
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

void Background::CompileShader(const char *shader_string, GLuint shader) {
    glShaderSource(shader, 1, &shader_string, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
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

int Background::Link(int program) {
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

}
