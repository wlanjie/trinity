#include "gpu_texture_frame_copier.h"

#define LOG_TAG "GPUTextureFrameCopier"

GPUTextureFrameCopier::GPUTextureFrameCopier() {
	vertex_shader_ = const_cast<char *>(NO_FILTER_VERTEX_SHADER);
	fragment_shader_ = const_cast<char *>(GPU_FRAME_FRAGMENT_SHADER);
}

GPUTextureFrameCopier::~GPUTextureFrameCopier() {
}

bool GPUTextureFrameCopier::Init() {
	prog_id_ = loadProgram(vertex_shader_, fragment_shader_);
	if (!prog_id_) {
		LOGE("Could not create program.");
		return false;
	}
	vertex_coords_ = glGetAttribLocation(prog_id_, "vPosition");
	checkGlError("glGetAttribLocation vPosition");
	texture_coords_ = glGetAttribLocation(prog_id_, "vTexCords");
	checkGlError("glGetAttribLocation vTexCords");
	uniform_texture_ = glGetUniformLocation(prog_id_, "yuvTexSampler");
	checkGlError("glGetAttribLocation yuvTexSampler");

	uniform_texture_matrix_ = glGetUniformLocation(prog_id_, "texMatrix");
	checkGlError("glGetUniformLocation uniform_texture_matrix_");

	uniform_transforms_ = glGetUniformLocation(prog_id_, "trans");
	checkGlError("glGetUniformLocation uniform_transforms_");

	initialized_ = true;
	return true;
}

void GPUTextureFrameCopier::RenderWithCoords(TextureFrame *textureFrame, GLuint texId, GLfloat *vertexCoords,
											 GLfloat *textureCoords) {
	glBindTexture(GL_TEXTURE_2D, texId);
	checkGlError("glBindTexture");

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			texId, 0);
	checkGlError("glFramebufferTexture2D");

	glUseProgram(prog_id_);
	if (!initialized_) {
		return;
	}

	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, GL_FALSE, 0, vertexCoords);
	glEnableVertexAttribArray (vertex_coords_);
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, GL_FALSE, 0, textureCoords);
	glEnableVertexAttribArray (texture_coords_);
	/* Binding the input texture */
    textureFrame->BindTexture(&uniform_texture_);

	float texTransMatrix[4 * 4];
	matrixSetIdentityM(texTransMatrix);
	glUniformMatrix4fv(uniform_texture_matrix_, 1, GL_FALSE, (GLfloat *) texTransMatrix);

	float rotateMatrix[4 * 4];
	matrixSetIdentityM(rotateMatrix);
	glUniformMatrix4fv(uniform_transforms_, 1, GL_FALSE, (GLfloat *) rotateMatrix);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(vertex_coords_);
    glDisableVertexAttribArray(texture_coords_);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
//    LOGI("Draw waste time is %ld", (getCurrentTime() - startDrawTimeMills));
}
