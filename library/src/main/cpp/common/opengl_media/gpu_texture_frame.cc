//
// Created by wlanjie on 2019/2/22.
//

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "gpu_texture_frame.h"
#include "../../log.h"

namespace trinity {

GPUTextureFrame::GPUTextureFrame() {
    decode_texture_id_ = 0;
}

GPUTextureFrame::~GPUTextureFrame() {

}

bool GPUTextureFrame::CreateTexture() {
    decode_texture_id_ = 0;
    int ret = InitTexture();
    if (ret < 0) {
        LOGE("InitTexture failed: %d", ret);
        Dealloc();
        return false;
    }
    return true;
}

void GPUTextureFrame::UpdateTextureImage() {
//TODO:调用surfaceTexture
}

bool GPUTextureFrame::BindTexture(GLint *uniform_samplers) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, decode_texture_id_);
    glUniform1i(*uniform_samplers, 0);
    return true;
}

void GPUTextureFrame::Dealloc() {
    if (decode_texture_id_) {
        glDeleteTextures(1, &decode_texture_id_);
    }
}

int GPUTextureFrame::InitTexture() {
    glGenTextures(1, &decode_texture_id_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, decode_texture_id_);
    if (checkGlError("glBindTexture")) {
        return -1;
    }
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (checkGlError("glTexParameteri")) {
        return -1;
    }
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if (checkGlError("glTexParameteri")) {
        return -1;
    }
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    if (checkGlError("glTexParameteri")) {
        return -1;
    }
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (checkGlError("glTexParameteri")) {
        return -1;
    }
    return 1;
}

}