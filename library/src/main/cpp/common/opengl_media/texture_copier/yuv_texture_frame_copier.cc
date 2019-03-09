#include "yuv_texture_frame_copier.h"

#define LOG_TAG "YUVTextureFrameCopier"

YUVTextureFrameCopier::YUVTextureFrameCopier() {
	vertex_shader_ = const_cast<char *>(NO_FILTER_VERTEX_SHADER);
	fragment_shader_ = const_cast<char *>(YUV_FRAME_FRAGMENT_SHADER);
}

YUVTextureFrameCopier::~YUVTextureFrameCopier() {
}

bool YUVTextureFrameCopier::Init() {
	prog_id_ = loadProgram(vertex_shader_, fragment_shader_);
	if (!prog_id_) {
		LOGE("Could not create program.");
		return false;
	}
	vertex_coords_ = glGetAttribLocation(prog_id_, "vPosition");
	checkGlError("glGetAttribLocation vPosition");
	texture_coords_ = glGetAttribLocation(prog_id_, "vTexCords");
	checkGlError("glGetAttribLocation vTexCords");
	glUseProgram(prog_id_);
	_uniformSamplers[0] = glGetUniformLocation(prog_id_, "s_texture_y");
	_uniformSamplers[1] = glGetUniformLocation(prog_id_, "s_texture_u");
	_uniformSamplers[2] = glGetUniformLocation(prog_id_, "s_texture_v");

	uniform_transforms_ = glGetUniformLocation(prog_id_, "trans");
	checkGlError("glGetUniformLocation uniform_transforms_");

	uniform_texture_matrix_ = glGetUniformLocation(prog_id_, "texMatrix");
	checkGlError("glGetUniformLocation uniform_texture_matrix_");

	initialized_ = true;
	return true;
}

void YUVTextureFrameCopier::RenderWithCoords(TextureFrame *textureFrame, GLuint texId, GLfloat *vertexCoords,
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
    textureFrame->BindTexture(_uniformSamplers);

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
