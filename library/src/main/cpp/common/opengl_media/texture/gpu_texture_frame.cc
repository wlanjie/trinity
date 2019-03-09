#include "gpu_texture_frame.h"

#define LOG_TAG "GPUTextureFrame"

GPUTextureFrame::GPUTextureFrame() {
	decodeTexId = 0;
}

GPUTextureFrame::~GPUTextureFrame() {

}

bool GPUTextureFrame::createTexture() {
	LOGI("enter GPUTextureFrame::createTexture");
	decodeTexId = 0;
	int ret = initTexture();
	if (ret < 0) {
		LOGI("Init texture failed...");
		this->dealloc();
		return false;
	}
	return true;
}

void GPUTextureFrame::updateTexImage() {
//TODO:调用surfaceTexture
}

bool GPUTextureFrame::bindTexture(GLint* uniformSamplers) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, decodeTexId);
    glUniform1i(*uniformSamplers, 0);
	return true;
}

void GPUTextureFrame::dealloc() {
	LOGI("enter GPUTextureFrame::Destroy");
	if (decodeTexId) {
		glDeleteTextures(1, &decodeTexId);
	}
}

int GPUTextureFrame::initTexture() {
	glGenTextures(1, &decodeTexId);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, decodeTexId);
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
