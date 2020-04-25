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

#include <unistd.h>
#include "yuv_render.h"
#include "tools.h"
#include "android_xlog.h"
#include "gl.h"

#define STR(s) #s

namespace trinity {

const char *YUV_FRAME_FRAGMENT_SHADER =
        "varying highp vec2 textureCoordinate;\n"
        "uniform sampler2D texture_y;\n"
        "uniform sampler2D texture_u;\n"
        "uniform sampler2D texture_v;\n"
        "void main() { \n"
        "   highp float y = texture2D(texture_y, textureCoordinate).r;\n"
        "   highp float u = texture2D(texture_u, textureCoordinate).r - 0.5;\n"
        "   highp float v = texture2D(texture_v, textureCoordinate).r - 0.5;\n"
        "   highp float r = y + 1.402 * v;\n"
        "   highp float g = y - 0.344 * u - 0.714 * v;\n"
        "   highp float b = y + 1.772 * u;\n"
        "   gl_FragColor = vec4(r,g,b,1.0);\n"
        "}\n";

const char* NV21_FRAGMENT_SHADER =
        "varying highp vec2 textureCoordinate;\n"
        "uniform sampler2D texture_y;\n"
        "uniform sampler2D texture_u;\n"
        "void main() {\n"
        "   float y = texture2D(texture_y, textureCoordinate).r;\n"
        "   vec4 uv = texture2D(texture_u, textureCoordinate);\n"
        "   float u = uv.r - 0.5;\n"
        "   float v = uv.a - 0.5;\n"
        "   float r = y + 1.402 * v;\n"
        "   float g = y - 0.344 * u - 0.714 * v;\n"
        "   float b = y + 1.772 * u;\n"
        "   gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";

static GLfloat VERTEX_COORDINATE[8] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
};

static GLfloat TEXTURE_COORDINATE_NO_ROTATION[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
};

static GLfloat TEXTURE_COORDINATE_ROTATED_90[8] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
};

static GLfloat TEXTURE_COORDINATE_ROTATED_180[8] = {
        1.0, 1.0,
        0.0, 1.0,
        1.0, 0.0,
        0.0, 0.0,
};
static GLfloat TEXTURE_COORDINATE_ROTATED_270[8] = {
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f
};

YuvRender::YuvRender()
    : program_(0)
    , texture_id_(0)
    , frame_buffer_id_(0)
    , textures()
    , uniform_samplers_()
    , vertex_coordinate_location_(0)
    , texture_coordinate_location_(0)
    , matrix_location_(0)
    , y(nullptr)
    , u(nullptr)
    , v(nullptr) {
    y_size_ = 0;
    u_size_ = 0;
    v_size_ = 0;
}

YuvRender::~YuvRender() {
    Destroy();
    if (nullptr != y) {
        delete[] y;
        y = nullptr;
    }
    if (nullptr != u) {
        delete[] u;
        u = nullptr;
    }
    if (nullptr != v) {
        delete[] v;
        v = nullptr;
    }
}

int YuvRender::Initialize(int width, int height, const char* fragment_shader) {
    CreateProgram(DEFAULT_VERTEX_MATRIX_SHADER, fragment_shader);
    if (program_ == 0) {
        return -1;
    }
    vertex_coordinate_location_ = static_cast<GLuint>(glGetAttribLocation(program_, "position"));
    texture_coordinate_location_ = static_cast<GLuint>(
            glGetAttribLocation(program_, "inputTextureCoordinate"));
    matrix_location_ = static_cast<GLuint>(glGetUniformLocation(program_, "textureMatrix"));
    glUseProgram(program_);
    uniform_samplers_[0] = glGetUniformLocation(program_, "texture_y");
    uniform_samplers_[1] = glGetUniformLocation(program_, "texture_u");
    uniform_samplers_[2] = glGetUniformLocation(program_, "texture_v");

    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
    }

    if (frame_buffer_id_ != 0) {
        glDeleteFramebuffers(1, &frame_buffer_id_);
    }

    glGenTextures(3, textures);
    for (int i = 0; i < 3; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenFramebuffers(1, &frame_buffer_id_);
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return 0;
}

void YuvRender::Destroy() {
    for (int i = 0; i < 3; i++) {
        GLuint texture = textures[i];
        glDeleteTextures(1, &texture);
        textures[i] = 0;
    }
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }

    if (frame_buffer_id_ != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &frame_buffer_id_);
        frame_buffer_id_ = 0;
    }

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void YuvRender::CreateProgram(const char *vertex, const char *fragment) {
    program_ = glCreateProgram();
    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    CompileShader(vertex, vertexShader);
    CompileShader(fragment, fragmentShader);
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    Link();
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void YuvRender::CompileShader(const char *shader_string, GLuint shader) {
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
                glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                LOGE("Could not compile %d:\n%s\n", shader, buf);
                free(buf);
            }
            glDeleteShader(shader);
        }
    }
}

void YuvRender::Link() {
    glLinkProgram(program_);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint infoLen;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
            if (buf) {
                glGetProgramInfoLog(program_, infoLen, nullptr, buf);
                printf("%s", buf);
                free(buf);
            }
            glDeleteProgram(program_);
            program_ = 0;
        }
    }
}

GLuint YuvRender::DrawFrame(AVFrame* frame, const GLfloat* matrix,
        const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate) {
    if (program_ == 0) {
        switch (frame->format) {
            case AV_PIX_FMT_YUV420P:
                Initialize(frame->width, frame->height, YUV_FRAME_FRAGMENT_SHADER);
                break;

            case AV_PIX_FMT_NV12:
                Initialize(frame->width, frame->height, NV21_FRAGMENT_SHADER);
                break;

            default:
                return 0;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    int frame_width = frame->width;
    int frame_height = frame->height;
    if (frame_width % 16 != 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, frame_width, frame_height);
    glUseProgram(program_);
    switch (frame->format) {
        case AV_PIX_FMT_YUV420P:
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glUniform1i(uniform_samplers_[0], 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            glUniform1i(uniform_samplers_[1], 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[1], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[1]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textures[2]);
            glUniform1i(uniform_samplers_[2], 2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[2], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[2]);
            break;

        case AV_PIX_FMT_NV12:
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glUniform1i(uniform_samplers_[0], 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            glUniform1i(uniform_samplers_[1], 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, frame->linesize[1] / 2, frame->height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, frame->data[1]);
            break;

        default:
            break;
    }

    glVertexAttribPointer(vertex_coordinate_location_, 2, GL_FLOAT, GL_FALSE, 0, vertex_coordinate);
    glEnableVertexAttribArray(vertex_coordinate_location_);
    glVertexAttribPointer(texture_coordinate_location_, 2, GL_FLOAT, GL_FALSE, 0, texture_coordinate);
    glEnableVertexAttribArray(texture_coordinate_location_);
    glUniformMatrix4fv(matrix_location_, 1, GL_FALSE, matrix);
    for (int i = 0; i < 3; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glUniform1i(uniform_samplers_[i], i);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(vertex_coordinate_location_);
    glDisableVertexAttribArray(texture_coordinate_location_);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return texture_id_;
}

void YuvRender::CopyFrame(uint8_t *dst, uint8_t *src, int width, int height, int line_size) {
    for (int i = 0; i < height; ++i) {
        memcpy(dst, src, width);
        dst += width;
        src += line_size;
    }
}

}  // namespace trinity
