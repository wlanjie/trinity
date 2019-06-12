//
// Created by wlanjie on 2019/4/13.
//

#include <malloc.h>
#include "opengl.h"
#include "android_xlog.h"
#include "matrix.h"
#include "gl.h"

namespace trinity {

static const GLfloat defaultVertexCoordinates[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
};

static const GLfloat defaultTextureCoordinate[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
};

OpenGL::OpenGL() {
    type_ = TEXTURE_2D;
    width_ = 0;
    height_ = 0;
    program_ = 0;
    CreateProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
}

OpenGL::OpenGL(int width, int height) {
    type_ = TEXTURE_2D;
    width_ = width;
    height_ = height;
    program_ = 0;
    CreateProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
}

OpenGL::OpenGL(const char *vertex, const char *fragment) {
    type_ = TEXTURE_2D;
    width_ = 0;
    height_ = 0;
    program_ = 0;
    CreateProgram(vertex, fragment);
}

OpenGL::OpenGL(int width, int height, const char *vertex, const char *fragment) {
    type_ = TEXTURE_2D;
    this->width_ = width;
    this->height_ = height;
    program_ = 0;
    CreateProgram(vertex, fragment);
}

OpenGL::~OpenGL() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void OpenGL::SetTextureType(trinity::TextureType type) {
    type_ = type;
}

void OpenGL::Init(const char* vertex, const char* fragment) {
    CreateProgram(vertex, fragment);
}

void OpenGL::SetOutput(int width, int height) {
    this->width_ = width;
    this->height_ = height;
}

void OpenGL::SetInt(const char *name, int value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform1i(location, value);
}

void OpenGL::SetFloat(const char *name, float value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform1f(location, value);
}

void OpenGL::SetFloatVec2(const char *name, int size, const GLfloat *value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform2fv(location, size, value);
}

void OpenGL::SetFloatVec3(const char *name, int size, const GLfloat *value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform3fv(location, size, value);
}

void OpenGL::SetFloatVec4(const char *name, int size, const GLfloat *value) {
    GLint location = glGetUniformLocation(program_, name);
    glUniform4fv(location, size, value);
}

void OpenGL::SetUniformMatrix3f(const char *name, int size, const GLfloat *matrix) {
    GLint location = glGetUniformLocation(program_, name);
    glUniformMatrix3fv(location, size, GL_FALSE, matrix);
}

void OpenGL::SetUniformMatrix4f(const char *name, int size, const GLfloat *matrix) {
    GLint location = glGetUniformLocation(program_, name);
    glUniformMatrix4fv(location, size, GL_FALSE, matrix);
}

void OpenGL::ProcessImage(GLuint texture_id) {
    ProcessImage(texture_id, defaultVertexCoordinates, defaultTextureCoordinate);
}

void OpenGL::ProcessImage(GLuint texture_id, GLfloat *texture_matrix) {
    ProcessImage(texture_id, defaultVertexCoordinates, defaultTextureCoordinate, texture_matrix);
}

void OpenGL::ProcessImage(GLuint texture_id, const GLfloat *vertex_coordinate, const GLfloat *texture_coordinate) {
    float texTransMatrix[4 * 4];
    matrixSetIdentityM(texTransMatrix);
    ProcessImage(texture_id, vertex_coordinate, texture_coordinate, texTransMatrix);
}

void OpenGL::ProcessImage(GLuint texture_id, const GLfloat *vertex_coordinate, const GLfloat *texture_coordinate,
                          GLfloat *texture_matrix) {
    if (program_ == 0) {
        // create program failed.
        return;
    }
    glUseProgram(program_);
    if (width_ > 0 && height_ > 0) {
        glViewport(0, 0, width_, height_);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RunOnDrawTasks();
    GLuint positionAttribute = static_cast<GLuint>(glGetAttribLocation(program_, "position"));
    glEnableVertexAttribArray(positionAttribute);
    glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertex_coordinate);
    GLuint textureCoordinateAttribute = static_cast<GLuint>(glGetAttribLocation(program_, "inputTextureCoordinate"));
    glEnableVertexAttribArray(textureCoordinateAttribute);
    glVertexAttribPointer(textureCoordinateAttribute, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texture_coordinate);
    SetUniformMatrix4f("textureMatrix", 1, texture_matrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(type_ == TEXTURE_OES ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D, texture_id);
    SetInt("inputImageTexture", 0);
    OnDrawArrays();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(positionAttribute);
    glDisableVertexAttribArray(textureCoordinateAttribute);
    glBindTexture(type_ == TEXTURE_OES ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D, 0);
}

void OpenGL::RunOnDrawTasks() {

}

void OpenGL::OnDrawArrays() {

}

void OpenGL::CreateProgram(const char *vertex, const char *fragment) {
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

void OpenGL::CompileShader(const char *shader_string, GLuint shader) {
    glShaderSource(shader, 1, &shader_string, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = (char *) malloc((size_t) infoLen);
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

void OpenGL::Link() {
    glLinkProgram(program_);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint infoLen;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = (char *) malloc((size_t) infoLen);
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
