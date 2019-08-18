//
// Created by wlanjie on 2019/4/13.
//

#include "frame_buffer.h"
#include "size.h"
#include "android_xlog.h"

namespace trinity {

FrameBuffer::FrameBuffer(int width, int height, const char *vertex, const char *fragment) : OpenGL(width, height, vertex, fragment) {
    start_time_ = 0;
    end_time_ = INT64_MAX;
    CompileFrameBuffer(0, 0, width, height);
}

FrameBuffer::FrameBuffer(int width, int height, int view_width, int view_height, const char* vertex, const char* fragment) : OpenGL(width, height, vertex, fragment) {
    start_time_ = 0;
    end_time_ = INT64_MAX;
    CompileFrameBuffer(view_width, view_height, width, height);
}

FrameBuffer::FrameBuffer(int render_type, int width, int height, int view_width, int view_height, const char *vertex,
                         const char *fragment) : OpenGL(width, height, vertex, fragment) {
    start_time_ = 0;
    end_time_ = INT64_MAX;
    CompileFrameBuffer(view_width, view_height, width, height);
    SetRenderType(render_type);
}

void FrameBuffer::CompileFrameBuffer(int view_width, int view_height, int camera_width, int camera_height) {
    view_width_ = view_width;
    view_height_ = view_height;
    camera_width_ = camera_width;
    camera_height_ = camera_height;

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

void FrameBuffer::SetStartTime(uint64_t time) {
    start_time_ = time;
}

void FrameBuffer::SetEndTime(uint64_t time) {
    end_time_ = time;
}

void FrameBuffer::SetRenderType(int type) {
    ResetVertexCoordinate();
    LOGE("type: %d view_width: %d view_height: %d camera_width: %d camera_height: %d", type, view_width_, view_height_, camera_width_, camera_height_);
    float w;
    float h;
    float src_scale = camera_width_ * 1.0f / camera_height_;
    float screen_scale = view_width_ * 1.0f / view_height_;
    if (src_scale == screen_scale) {
        return;
    }
    if (view_width_ > 0 && view_height_> 0) {
        if (type == CROP) {
            if (src_scale > screen_scale) {
                w = 1.0f;
                h = screen_scale * 1.0f / src_scale;
            } else {
                w = src_scale * 1.0f / screen_scale;
                h = 1.0f;
            }
            LOGE("w: %f h: %f", w, h);
            vertex_coordinate_[0] = -w;
            vertex_coordinate_[1] = -h;
            vertex_coordinate_[2] = w;
            vertex_coordinate_[3] = -h;
            vertex_coordinate_[4] = -w;
            vertex_coordinate_[5] = h;
            vertex_coordinate_[6] = w;
            vertex_coordinate_[7] = h;
        } else if (type == FIX_XY) {
            if (src_scale > screen_scale) {
                w = screen_scale * 1.0f / src_scale;
                h = 0.0f;
            } else {
                w = 0.0f;
                h = src_scale * 1.0f / screen_scale;
            }

            texture_coordinate_[0] = 1.0f - w;
            texture_coordinate_[1] = 0.0f;
            texture_coordinate_[2] = w;
            texture_coordinate_[3] = 0.0f;
            texture_coordinate_[4] = 1.0f - w;
            texture_coordinate_[5] = 1.0f;
            texture_coordinate_[6] = w;
            texture_coordinate_[7] = 1.0f;

            for (int i = 0; i < 8; ++i) {
                LOGE("i: %f", texture_coordinate_[i]);
            }
        }
    }
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