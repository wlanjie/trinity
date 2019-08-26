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
#include "matrix.h"
#include "tools.h"
#include "android_xlog.h"
#include "gl.h"

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


static GLfloat VERTEX_COORDINATE[8] = {
        -1.0f, -1.0f,    // 0 top left
        1.0f, -1.0f,    // 1 bottom left
        -1.0f, 1.0f,  // 2 bottom right
        1.0f, 1.0f,    // 3 top right
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

void YuvRender::ConvertVertexCoordinate(int width, int height, int view_width, int view_height) {
//    float w;
//    float h;
//    float src_scale = width *1.0f / height;
//    float screen_scale = view_width *1.0f / view_height;
//
//    if (src_scale == screen_scale) return;
//
//    if (src_scale > screen_scale) {
//        w = 1.0f;
//        h = screen_scale * 1.0f / src_scale;
//    } else {
//        w = src_scale * 1.0f / screen_scale;
//        h = 1.0f;
//    }
//
//    VERTEX_COORDINATE[0] = -w;
//    VERTEX_COORDINATE[1] = -h;
//
//    VERTEX_COORDINATE[2] =  w;
//    VERTEX_COORDINATE[3] = -h;
//
//    VERTEX_COORDINATE[4] = -w;
//    VERTEX_COORDINATE[5] =  h;
//
//    VERTEX_COORDINATE[6] =  w;
//    VERTEX_COORDINATE[7] =  h;
}

YuvRender::YuvRender(int width, int height, int view_width, int view_height, int degree) {
    program_ = 0;
    texture_id_ = 0;
    frame_buffer_id_ = 0;
    vertex_coordinate_ = new GLfloat[VERTEX_COORDINATE_COUNT];
    texture_coordinate_ = new GLfloat[VERTEX_COORDINATE_COUNT];

    if (view_width > 0 && view_height > 0) {
        ConvertVertexCoordinate(width, height, view_width, view_height);
    }

    memcpy(vertex_coordinate_, VERTEX_COORDINATE, sizeof(GLfloat) * VERTEX_COORDINATE_COUNT);
    switch (degree) {
        case 90:
            memcpy(texture_coordinate_, TEXTURE_COORDINATE_ROTATED_90, sizeof(GLfloat) * VERTEX_COORDINATE_COUNT);
            break;
        case 180:
            memcpy(texture_coordinate_, TEXTURE_COORDINATE_ROTATED_180, sizeof(GLfloat) * VERTEX_COORDINATE_COUNT);
            break;
        case 270:
            memcpy(texture_coordinate_, TEXTURE_COORDINATE_ROTATED_270, sizeof(GLfloat) * VERTEX_COORDINATE_COUNT);
            break;
        default:
            memcpy(texture_coordinate_, TEXTURE_COORDINATE_NO_ROTATION, sizeof(GLfloat) * VERTEX_COORDINATE_COUNT);
            break;
    }

    Initialize(width, height);

    y = nullptr;
    u = nullptr;
    v = nullptr;
    y_size_ = 0;
    u_size_ = 0;
    v_size_ = 0;
}

YuvRender::~YuvRender() {
    Destroy();
    delete[] vertex_coordinate_;
    delete[] texture_coordinate_;
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

int YuvRender::Initialize(int width, int height) {
    CreateProgram(DEFAULT_VERTEX_SHADER, YUV_FRAME_FRAGMENT_SHADER);
    if (program_ == 0) {
        return -1;
    }
    vertex_coordinate_location_ = glGetAttribLocation(program_, "position");
    texture_coordinate_location_ = glGetAttribLocation(program_, "inputTextureCoordinate");
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

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

int YuvRender::DrawFrame(AVFrame *frame) {
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);

    int frame_width = frame->width;
    int frame_height = frame->height;
    if (frame_width % 16 != 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    int widths[3] = {frame_width, frame_width >> 1, frame_width >> 1};
    int heights[3] = {frame_height, frame_height >> 1, frame_height >> 1};

    int width = MIN(frame->linesize[0], frame_width);
    int height = frame_height;
    int y_size = width * height;
    if (y_size_ != y_size) {
        if (nullptr != y) {
            delete[] y;
        }
        y_size_ = y_size;
        y = new uint8_t[y_size_];
    }
    CopyFrame(y, frame->data[0], width, height, frame->linesize[0]);

    width = MIN(frame->linesize[1], frame_width >> 1);
    height = frame_height >> 1;

    int u_size = y_size >> 2;
    if (u_size_ != u_size) {
        if (u != nullptr) {
            delete[] u;
        }
        u_size_ = u_size;
        u = new uint8_t[u_size_];
    }
    CopyFrame(u, frame->data[1], width, height, frame->linesize[1]);

    width = MIN(frame->linesize[2], frame_width >> 1);
    height = frame_height >> 1;

    int v_size = y_size >> 2;
    if (v_size_ != v_size) {
        if (v != nullptr) {
            delete[] v;
        }
        v_size_ = v_size;
        if (v == nullptr) {
            v = new uint8_t[v_size_];
        }
    }
    CopyFrame(v, frame->data[2], width, height, frame->linesize[2]);

    uint8_t *pixels[3] = {y, u, v};

    for (int i = 0; i < 3; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widths[i], heights[i], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                     pixels[i]);
    }

    glViewport(0, 0, frame_width, frame_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);
    glUseProgram(program_);
    glVertexAttribPointer(vertex_coordinate_location_, 2, GL_FLOAT, GL_FALSE, 0, vertex_coordinate_);
    glEnableVertexAttribArray(vertex_coordinate_location_);
    glVertexAttribPointer(texture_coordinate_location_, 2, GL_FLOAT, GL_FALSE, 0, texture_coordinate_);
    glEnableVertexAttribArray(texture_coordinate_location_);

    for (int i = 0; i < 3; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glUniform1i(uniform_samplers_[i], i);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(vertex_coordinate_location_);
    glDisableVertexAttribArray(texture_coordinate_location_);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
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
