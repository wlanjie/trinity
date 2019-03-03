#include "recording_preview_renderer.h"

#define LOG_TAG "RecordingPreviewRenderer"

RecordingPreviewRenderer::RecordingPreviewRenderer() {
	degress_ = 270;
	camera_texutre_frame_ = NULL;
	copier_ = NULL;
	texture_coords_ = NULL;
	texture_coords_size_ = 8;
	renderer_ = NULL;
}

RecordingPreviewRenderer::~RecordingPreviewRenderer() {
}

void RecordingPreviewRenderer::SetDegress(int degress, bool isVFlip) {
	this->degress_ = degress;
	this->is_vflip_ = isVFlip;
	this->FillTextureCoords();
}

void RecordingPreviewRenderer::Init(int degress, bool isVFlip, int textureWidth, int textureHeight,
                                    int cameraWidth, int cameraHeight) {
	LOGI("enter RecordingPreviewRenderer::Init() texture_width_ is %d, texture_height_ is %d", textureWidth, textureHeight);
	this->degress_ = degress;
	this->is_vflip_ = isVFlip;
	texture_coords_ = new GLfloat[texture_coords_size_];
	this->FillTextureCoords();
	this->camera_width_ = cameraWidth;
	this->camera_height_ = cameraHeight;

	copier_ = new GPUTextureFrameCopier();
	copier_->init();
	renderer_ = new VideoGLSurfaceRender();
	renderer_->init(textureWidth, textureHeight);
	camera_texutre_frame_ = new GPUTextureFrame();
	camera_texutre_frame_->createTexture();
	glGenTextures(1, &input_texture_id_);
	checkGlError("glGenTextures input_texture_id_");
	glBindTexture(GL_TEXTURE_2D, input_texture_id_);
	checkGlError("glBindTexture input_texture_id_");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLint internalFormat = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei) textureWidth, (GLsizei) textureHeight, 0, internalFormat, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &fbo_);
	checkGlError("glGenFramebuffers");

	glGenTextures(1, &rotate_texture_id_);
	checkGlError("glGenTextures rotate_texture_id_");
	glBindTexture(GL_TEXTURE_2D, rotate_texture_id_);
	checkGlError("glBindTexture rotate_texture_id_");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (degress == 90 || degress == 270)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cameraHeight, cameraWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cameraWidth, cameraHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	LOGI("leave RecordingPreviewRenderer::Init()");
}

void RecordingPreviewRenderer::ProcessFrame(float position) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	checkGlError("glBindFramebuffer fbo_");

	if (degress_ == 90 || degress_ == 270)
		glViewport(0, 0, camera_height_, camera_width_);
	else
		glViewport(0, 0, camera_width_, camera_height_);

	GLfloat* vertexCoords = this->GetVertexCoords();
	copier_->renderWithCoords(camera_texutre_frame_, rotate_texture_id_, vertexCoords, texture_coords_);

	int rotateTexWidth = camera_width_;
	int rotateTexHeight = camera_height_;
	if (degress_ == 90 || degress_ == 270){
		rotateTexWidth = camera_height_;
		rotateTexHeight = camera_width_;
	}
	renderer_->renderToAutoFitTexture(rotate_texture_id_, rotateTexWidth, rotateTexHeight, input_texture_id_);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RecordingPreviewRenderer::DrawToView(int videoWidth, int videoHeight) {
	renderer_->renderToView(input_texture_id_, videoWidth, videoHeight);
}

void RecordingPreviewRenderer::DrawToViewWithAutoFit(int videoWidth, int videoHeight, int texWidth,
													 int texHeight) {
	renderer_->renderToViewWithAutofit(input_texture_id_, videoWidth, videoHeight, texWidth, texHeight);
}

int RecordingPreviewRenderer::GetCameraTexId() {
	if (camera_texutre_frame_) {
		return camera_texutre_frame_->getDecodeTexId();
	}
	return -1;
}

void RecordingPreviewRenderer::Destroy(){
	LOGI("enter RecordingPreviewRenderer::Destroy()");
	if(copier_){
		copier_->destroy();
		delete copier_;
		copier_ = NULL;
	}
	LOGI("after delete copier_");
	if(renderer_) {
		LOGI("delete renderer_..");
		renderer_->dealloc();
		delete renderer_;
		renderer_ = NULL;
	}
	LOGI("after delete renderer_");
	if (input_texture_id_) {
		LOGI("delete input_texture_id_ ..");
		glDeleteTextures(1, &input_texture_id_);
	}
	if(fbo_){
		LOGI("delete fbo_..");
		glDeleteBuffers(1, &fbo_);
	}
	if (camera_texutre_frame_ != NULL){
		camera_texutre_frame_->dealloc();
		delete camera_texutre_frame_;
		camera_texutre_frame_ = NULL;
	}

	if(texture_coords_){
		delete[] texture_coords_;
		texture_coords_ = NULL;
	}
	LOGI("leave RecordingPreviewRenderer::Destroy()");
}

void RecordingPreviewRenderer::FillTextureCoords() {
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

float RecordingPreviewRenderer::Flip(float i) {
	if (i == 0.0f) {
		return 1.0f;
	}
	return 0.0f;
}

GLfloat* RecordingPreviewRenderer::GetVertexCoords() {
	return CAMERA_TRIANGLE_VERTICES;
}
