//
// Created by wlanjie on 2019/2/22.
//

#include <cstring>
#include "preview_renderer.h"
#include "../common/opengl_media/gpu_texture_frame.h"
#include "../common/egl/gl_tools.h"

namespace trinity {

PreviewRenderer::PreviewRenderer() {
    degress_ = 270;
    camera_texture_frame_ = nullptr;
    copier_ = nullptr;
    texture_coords_ = nullptr;
    texture_coords_size_ = 8;
    render_ = nullptr;
}

PreviewRenderer::~PreviewRenderer() {

}

void PreviewRenderer::Init(int degress, bool is_vflip, int texture_width, int texture_height, int camera_width,
                           int camera_height) {
    degress_ = degress;
    is_vflip_ = is_vflip;
    texture_coords_ = new GLfloat[texture_coords_size_];
    FillTextureCoords();
    texture_width_ = texture_width;
    texture_height_ = texture_height;
    camera_width_ = camera_width;
    camera_height_ = camera_height;

    copier_ = new GPUTextureFrameCopier();
    copier_->Init();
    render_ = new VideoGLSurfaceRender();
    render_->Init(texture_width, texture_height);
    camera_texture_frame_ = new GPUTextureFrame();
    camera_texture_frame_->CreateTexture();
    glGenTextures(1, &input_texture_id_);
    checkGlError("glGenTextures inputTextureId");
    glBindTexture(GL_TEXTURE_2D, input_texture_id_);
    checkGlError("glBindTexture inputTextureId");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLint internalFormat = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei) texture_width, (GLsizei) texture_height, 0, internalFormat, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fbo_);
    checkGlError("glGenFrameBuffers");

    glGenTextures(1, &rotate_texture_id_);
    checkGlError("glGenTextures rotateTexId");
    glBindTexture(GL_TEXTURE_2D, rotate_texture_id_);
    checkGlError("glBindTexture rotateTexId");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (degress == 90 || degress == 270)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, camera_height, camera_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, camera_width, camera_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("leave RecordingPreviewRenderer::init()");
}

void PreviewRenderer::SetDegress(int degress, bool is_vflip) {
    this->degress_ = degress;
    this->is_vflip_ = is_vflip;
    this->FillTextureCoords();
}

void PreviewRenderer::ProcessFrame(float position) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    checkGlError("glBindFramebuffer FBO");

    if (degress_ == 90 || degress_ == 270)
        glViewport(0, 0, camera_height_, camera_width_);
    else
        glViewport(0, 0, camera_width_, camera_height_);

    GLfloat* vertexCoords = this->GetVertexCoords();
    copier_->RenderWithCoords(camera_texture_frame_, rotate_texture_id_, vertexCoords, texture_coords_);

    int rotateTexWidth = camera_width_;
    int rotateTexHeight = camera_height_;
    if (degress_ == 90 || degress_ == 270){
        rotateTexWidth = camera_height_;
        rotateTexHeight = camera_width_;
    }
    render_->RenderToAutoFitTexture(rotate_texture_id_, rotateTexWidth, rotateTexHeight, input_texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PreviewRenderer::DrawToView(int video_width, int video_height) {
    render_->RenderToView(input_texture_id_, video_width, video_height);
}

void PreviewRenderer::DrawToViewWithAutoFit(int video_width, int video_height, int texture_width, int texture_height) {
    render_->RenderToViewWithAutofit(input_texture_id_, video_width, video_height, texture_width, texture_height);
}

void PreviewRenderer::Dealloc() {
    if (copier_) {
        copier_->Destroy();
        delete copier_;
        copier_ = nullptr;
    }
    if (render_) {
        render_->Dealloc();
        delete render_;
        render_ = nullptr;
    }
    if (input_texture_id_) {
        glDeleteTextures(1, &input_texture_id_);
        input_texture_id_ = 0;
    }
    if (fbo_) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (camera_texture_frame_) {
        camera_texture_frame_->Dealloc();
        delete camera_texture_frame_;
        camera_texture_frame_ = nullptr;
    }
    if (texture_coords_) {
        delete[] texture_coords_;
        texture_coords_ = nullptr;
    }
}

int PreviewRenderer::GetCameraTextureId() {
    if (camera_texture_frame_) {
        return camera_texture_frame_->GetDecodeTextureId();
    }
    return -1;
}

void PreviewRenderer::FillTextureCoords() {
    GLfloat* tempTextureCoords;
    switch (degress_) {
        case 90:
            tempTextureCoords = CAMERA_TEXTURE_ROTATED_90;
            break;
        case 180:
            tempTextureCoords = CAMERA_TEXTURE_ROTATED_180;
            break;
        case 270:
            tempTextureCoords = CAMERA_TEXTURE_ROTATED_270;
            break;
        case 0:
        default:
            tempTextureCoords = CAMERA_TEXTURE_NO_ROTATION;
            break;
    }
    memcpy(texture_coords_, tempTextureCoords, texture_coords_size_ * sizeof(GLfloat));
    if(is_vflip_){
        texture_coords_[1] = Flip(texture_coords_[1]);
        texture_coords_[3] = Flip(texture_coords_[3]);
        texture_coords_[5] = Flip(texture_coords_[5]);
        texture_coords_[7] = Flip(texture_coords_[7]);
    }
}

float PreviewRenderer::Flip(float i) {
    if (i == 0.0f) {
        return 1.0f;
    }
    return 0.0f;
}

GLfloat *PreviewRenderer::GetVertexCoords() {
    return CAMERA_TRIANGLE_VERTICES;
}
}