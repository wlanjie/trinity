//
// Created by wlanjie on 2019/4/13.
//

#include "frame_buffer.h"
#include "size.h"
#include "android_xlog.h"

namespace trinity {

FrameBuffer::FrameBuffer(int width, int height, const char *vertex, const char *fragment) : OpenGL(width, height, vertex, fragment) {
    CompileFrameBuffer(width, height, vertex, fragment);
}

FrameBuffer::FrameBuffer(int width, int height, int view_width, int view_height, const char* vertex, const char* fragment) : OpenGL(width, height, vertex, fragment) {
    CompileFrameBuffer(width, height, vertex, fragment);

    if (view_width > 0 && view_height > 0) {
        float w;
        float h;
        float src_scale = width *1.0f / height;
        float screen_scale = view_width *1.0f / view_height;

        if (src_scale == screen_scale) return;

        if (src_scale > screen_scale) {
            w = 1.0f;
            h = screen_scale * 1.0f / src_scale;
        } else {
            w = src_scale * 1.0f / screen_scale;
            h = 1.0f;
        }

        vertex_coordinate_[0] = -w;
        vertex_coordinate_[1] = -h;

        vertex_coordinate_[2] =  w;
        vertex_coordinate_[3] = -h;

        vertex_coordinate_[4] = -w;
        vertex_coordinate_[5] =  h;

        vertex_coordinate_[6] =  w;
        vertex_coordinate_[7] =  h;
    }
}

void FrameBuffer::CompileFrameBuffer(int width, int height, const char *vertex, const char *fragment) {
    glGenTextures(1, &texture_id_);
    glGenFramebuffers(1, &frameBuffer_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("frame buffer error");
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];
    vertex_coordinate_[0] = -1.0f;
    vertex_coordinate_[1] = -1.0f;
    vertex_coordinate_[2] = 1.0f;
    vertex_coordinate_[3] = -1.0f;
    vertex_coordinate_[4] = -1.0f;
    vertex_coordinate_[5] = 1.0f;
    vertex_coordinate_[6] = 1.0f;
    vertex_coordinate_[7] = 1.0f;

    texture_coordinate_[0] = 0.0f;
    texture_coordinate_[1] = 0.0f;
    texture_coordinate_[2] = 1.0f;
    texture_coordinate_[3] = 0.0f;
    texture_coordinate_[4] = 0.0f;
    texture_coordinate_[5] = 1.0f;
    texture_coordinate_[6] = 1.0f;
    texture_coordinate_[7] = 1.0f;
}


FrameBuffer::~FrameBuffer() {
    glDeleteTextures(1, &texture_id_);
    glDeleteFramebuffers(1, &frameBuffer_id_);
}

//void FrameBuffer::Init(const char* vertex, const char* fragment) {
//    OpenGL::Init(vertex, fragment);
//}

GLuint FrameBuffer::OnDrawFrame(GLuint texture_id) {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
    ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return texture_id_;
}

GLuint FrameBuffer::OnDrawFrame(GLuint texture_id, GLfloat *matrix) {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
    ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_, matrix);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return texture_id_;
}

GLuint
FrameBuffer::OnDrawFrame(GLuint texture_id, const GLfloat *vertex_coordinate, const GLfloat *texture_coordinate) {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
    ProcessImage(texture_id, vertex_coordinate, texture_coordinate);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return texture_id_;
}

void FrameBuffer::SetFrame(int frame, int screen_width, int screen_height) {
    int width = 0;
    int height = 0;
    if (frame == FRAME_SQUARE) {
        width = 1;
        height = 1;
    }
    Size ratio = Size(width == 0 ? screen_width : width, height == 0 ? screen_height : height);
    Rect bounds = Rect(0, 0, screen_width, screen_height);
    Rect* rect = bounds.GetRectWithAspectRatio(ratio);
    float width_scale = rect->GetWidth() / bounds.GetWidth();
    float height_scale = rect->GetHeight() / bounds.GetHeight();
    ResetVertexCoordinate();
    for (int i = 0; i < 4; i++) {
        vertex_coordinate_[i * 2] = vertex_coordinate_[i * 2] * width_scale;
        vertex_coordinate_[i * 2 + 1] = vertex_coordinate_[i * 2 + 1] * height_scale;

        texture_coordinate_[i * 2] = texture_coordinate_[i * 2] * width_scale;
        texture_coordinate_[i * 2 + 1] = texture_coordinate_[i * 2 + 1] * height_scale;
    }
    delete rect;
}

void FrameBuffer::ResetVertexCoordinate() {
    vertex_coordinate_[0] = -1.0f;
    vertex_coordinate_[1] = -1.0f;
    vertex_coordinate_[2] = 1.0f;
    vertex_coordinate_[3] = -1.0f;
    vertex_coordinate_[4] = -1.0f;
    vertex_coordinate_[5] = 1.0f;
    vertex_coordinate_[6] = 1.0f;
    vertex_coordinate_[7] = 1.0f;

    texture_coordinate_[0] = 0.0f;
    texture_coordinate_[1] = 0.0f;
    texture_coordinate_[2] = 1.0f;
    texture_coordinate_[3] = 0.0f;
    texture_coordinate_[4] = 0.0f;
    texture_coordinate_[5] = 1.0f;
    texture_coordinate_[6] = 1.0f;
    texture_coordinate_[7] = 1.0f;
}

}