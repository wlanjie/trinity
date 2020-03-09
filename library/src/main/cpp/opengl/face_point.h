//
// Created by wlanjie on 2020-01-29.
//

#ifndef TRINITY_FACE_POINT_H
#define TRINITY_FACE_POINT_H

#include "opengl.h"

#if __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "android_xlog.h"
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__)
#endif

#include "face_detection.h"

static const char* FACE_POINT_VERTEX_SHADER =
        "attribute vec4 position;"
        "void main() {"
        "   gl_Position = position;"
        "   gl_PointSize = 12.0;"
        "}";

static const char* FACE_POINT_FRAGMENT_SHADER =
        "void main() {"
        "   gl_FragColor = vec4(1.0,0.0,0.0,1.0);"
        "}";

namespace trinity {

class FacePoint {
 public:
    FacePoint()
        : source_width_(0)
        , source_height_(0)
        , target_width_(0)
        , target_height_(0) {
        buffer_length_ = 111 * 2 * sizeof(float);
        face_point_buffer_ = new float[buffer_length_];
        CreateProgram(FACE_POINT_VERTEX_SHADER, FACE_POINT_FRAGMENT_SHADER);
        position_location_ = glGetAttribLocation(program_, "position");
        glGenBuffers(1, &vertex_buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER, buffer_length_, &vertex_buffer_, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    ~FacePoint() {
        if (program_ != 0) {
            glDeleteProgram(program_);
            program_ = 0;
        }
        delete[] face_point_buffer_;
    }

    void SetSourceSize(int width, int height) {
        source_width_ = width;
        source_height_ = height;
    }

    void SetTargetSize(int width, int height) {
        target_width_ = width;
        target_height_ = height;
    }

    float ConvertX(float x, int width) {
        float centerX = width / 2.0f;
        float t = x - centerX;
        return t / centerX;
    }

    float ConvertY(float y, int height) {
        float centerY = height / 2.0f;
        float s = centerY - y;
        return s / centerY;
    }

    void OnDrawFrame(std::vector<FaceDetectionReport*> faces) {
        for (FaceDetectionReport* face : faces) {
            if (face->left == 0 && face->right == 0 && face->top == 0 && face->bottom == 0) {
                continue;
            }
            GLfloat point[face->key_point_size];
            GLuint indices[face->key_point_size / 2];
//            GLuint indices[face->key_point_size / 2];

            for (uint i = 0; i < face->key_point_size / 2; i++) {
                float x = face->key_points[i * 2] / source_height_;
                // y镜像翻转一下
                float y = 1.0F - face->key_points[i * 2 + 1] / source_width_;
                point[i * 2] = x * 2 - 1;
                point[i * 2 + 1] = y * 2 - 1;
                indices[i] = i;
//                LOGE("point land_mark[%d] x: %f y: %f x1: %f y1: %f", i, x, y, x * 2 - 1, y * 2 - 1);
            }
//            LOGE("218: %f 219: %f 220: %f 221: %f sw: %f sh: %f\n", point[218], point[219], point[220], point[221], face->key_points[218], face->key_points[219]);
            glUseProgram(program_);
            glViewport(0, 0, target_width_, target_height_);
            glEnableVertexAttribArray(position_location_);
            glVertexAttribPointer(position_location_, 2, GL_FLOAT, GL_TRUE, 0, point);
            glDrawElements(GL_POINTS, sizeof(indices) / sizeof(GLuint), GL_UNSIGNED_INT, indices);
        }
    }

 private:
    void CreateProgram(const char* vertex, const char* fragment) {
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

    void CompileShader(const char* shader_string, GLuint shader) {
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

    void Link() {
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

 private:
    GLuint program_;
    GLint position_location_;
    GLuint vertex_buffer_;
    int buffer_length_;
    float* face_point_buffer_;
    int source_width_;
    int source_height_;
    int target_width_;
    int target_height_;
};

}

#endif //TRINITY_FACE_POINT_H
