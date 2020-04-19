//
// Created by wlanjie on 2019-12-19.
//

#include <cstdio>
#include <cstdlib>
#ifdef __ANDROID__
#include "android_xlog.h"
#else
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__) 
#endif
#include "process_buffer.h"

namespace trinity {

ProcessBuffer::ProcessBuffer()
    : program_(0)
    , width_(0)
    , height_(0)
    , default_vertex_coordinates_(nullptr)
    , default_texture_coordinates_(nullptr)
    , frame_buffer_id_(0)
    , texture_id_(0) {
}

ProcessBuffer::~ProcessBuffer() = default;

int ProcessBuffer::Init(const char *vertex_shader, const char *fragment_shader) {
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
    CreateProgram(vertex_shader, fragment_shader);

    glGenTextures(1, &texture_id_);
    glGenFramebuffers(1, &frame_buffer_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 720, 1280, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
    return 0;
}

void ProcessBuffer::SetOutput(int width, int height) {
    this->width_ = width;
    this->height_ = height;
}

void ProcessBuffer::SetInt(const char *name, int value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform1i(location, value);
}

void ProcessBuffer::SetFloat(const char *name, float value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform1f(location, value);
}

void ProcessBuffer::SetFloatVec2(const char *name, int size, const GLfloat *value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform2fv(location, size, value);

}

void ProcessBuffer::SetFloatVec3(const char *name, int size, const GLfloat *value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform3fv(location, size, value);
}

void ProcessBuffer::SetFloatVec4(const char *name, int size, const GLfloat *value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform4fv(location, size, value);
}

void ProcessBuffer::SetUniformMatrix3f(const char *name, int size, const GLfloat *matrix) {
    GLint location = glGetUniformLocation(program_, name);
    glUniformMatrix3fv(location, size, GL_FALSE, matrix);
}

void ProcessBuffer::SetUniformMatrix4f(const char *name, int size, const GLfloat *matrix) {
    GLint location = glGetUniformLocation(program_, name);
    glUniformMatrix4fv(location, size, GL_FALSE, matrix);
}

void ProcessBuffer::Destroy() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    if (frame_buffer_id_ != 0) {
        glDeleteFramebuffers(1, &frame_buffer_id_);
        frame_buffer_id_ = 0;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (nullptr != default_vertex_coordinates_) {
        delete[] default_vertex_coordinates_;
        default_vertex_coordinates_ = nullptr;
    }
    if (nullptr != default_texture_coordinates_) {
        delete[] default_texture_coordinates_;
        default_texture_coordinates_ = nullptr;
    }
}

void ProcessBuffer::ActiveProgram() {
    if (program_ == 0) {
        return;
    }
    glUseProgram(program_);
}

void ProcessBuffer::Clear() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (width_ > 0 && height_ > 0) {
        glViewport(0, 0, width_, height_);
    }
}

void ProcessBuffer::ActiveAttribute() {
    auto positionAttribute = static_cast<GLuint>(glGetAttribLocation(program_, "position"));
    glEnableVertexAttribArray(positionAttribute);
    glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), default_vertex_coordinates_);
    auto textureCoordinateAttribute = static_cast<GLuint>(glGetAttribLocation(program_, "inputTextureCoordinate"));
    glEnableVertexAttribArray(textureCoordinateAttribute);
    glVertexAttribPointer(textureCoordinateAttribute, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), default_texture_coordinates_);
}

void ProcessBuffer::DrawArrays() {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ProcessBuffer::CreateProgram(const char *vertex, const char *fragment) {
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

void ProcessBuffer::CompileShader(const char *shader_string, GLuint shader) {
    glShaderSource(shader, 1, &shader_string, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    LOGE("%s\n", shader_string);
    LOGE("%s\n", "==================");
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

void ProcessBuffer::Link() {
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

}
