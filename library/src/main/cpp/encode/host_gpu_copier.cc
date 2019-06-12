#include <malloc.h>
#include "host_gpu_copier.h"
#include "android_xlog.h"

static inline void checkGlError(const char* op){
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGE("after %s() glError (0x%x)\n", op, error);
	}
}


static inline GLuint loadShader(GLenum shaderType, const char* pSource) {
	GLuint shader = glCreateShader(shaderType);
	if (shader) {
		glShaderSource(shader, 1, &pSource, NULL);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				char* buf = (char*) malloc(infoLen);
				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					LOGI("Could not compile shader %d:\n%s\n", shaderType, buf);
					free(buf);
				}
			} else {
				LOGI( "Guessing at GL_INFO_LOG_LENGTH Size\n");
				char* buf = (char*) malloc(0x1000);
				if (buf) {
					glGetShaderInfoLog(shader, 0x1000, NULL, buf);
					LOGI("Could not compile shader %d:\n%s\n", shaderType, buf);
					free(buf);
				}
			}
			glDeleteShader(shader);
			shader = 0;
		}
	}
	return shader;
}

static inline GLuint loadProgram(const char* pVertexSource, const char* pFragmentSource) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
	if (!vertexShader) {
		return 0;
	}
	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
	if (!pixelShader) {
		return 0;
	}
	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexShader);
		checkGlError("glAttachShader");
		glAttachShader(program, pixelShader);
		checkGlError("glAttachShader");
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char* buf = (char*) malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					LOGI("Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}
	return program;
}

HostGPUCopier::HostGPUCopier() {
	prog_id_ = 0;
	download_texture_ = 0;
	this->recording_ = false;
}

HostGPUCopier::~HostGPUCopier() {
}

void HostGPUCopier::CopyYUY2Image(GLuint ipTex, uint8_t *yuy2ImageBuffer, int width, int height) {
	//将纹理渲染到YUY2格式的数据
//	LOGI("width_ is %d height_ is %d", width_, height_);
	this->ConvertYUY2(ipTex, width, height);
	//Download image
	const unsigned int yuy2Pairs = (width + 1) / 2;
	int imageBufferSize = GetBufferSizeInBytes(width, height);
	this->DownloadImageFromTexture(download_texture_, yuy2ImageBuffer, yuy2Pairs, height);
}

void HostGPUCopier::EnsureTextureStorage(GLint internalFormat, const unsigned int yuy2Pairs, int height) {
	if (!download_texture_) {
		glGenTextures(1, &download_texture_);
		checkGlError("glGenTextures download_texture_");
		glBindTexture(GL_TEXTURE_2D, download_texture_);
		checkGlError("glBindTexture download_texture_");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glBindTexture(GL_TEXTURE_2D, download_texture_);
		checkGlError("glBindTexture download_texture_");
	}
	// Specify texture
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, yuy2Pairs, (GLsizei) height, 0, internalFormat, GL_UNSIGNED_BYTE, 0);
}

void HostGPUCopier::ConvertYUY2(GLuint ipTex, int width, int height) {
	PrepareConvertToYUY2Program();

	const unsigned int yuy2Pairs = (width + 1) / 2;
	//GL_OES_required_internalFormat  0x8058 // GL_RGBA8/GL_RGBA8_OES
	GLint internalFormat = GL_RGBA;
	EnsureTextureStorage(internalFormat, yuy2Pairs, height);

	glActiveTexture(GL_TEXTURE0);
	checkGlError("glActiveTexture GL_TEXTURE0");
	glBindTexture(GL_TEXTURE_2D, ipTex);
	checkGlError("glBindTexture ipTex");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Attach YUY2 texture to frame_ buffer_
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	checkGlError("glBindFramebuffer");
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, download_texture_, 0);
	checkGlError("glFramebufferTexture2D");
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("GL_FRAMEBUFFER_COMPLETE false\n");
	}
	/**
	 * 首先glReadPixels读出来的格式是GL_RGBA,每一个像素是用四个字节表示
	 * 而我们期望的读出来的是YUY2
	 * width:4 height:4
	 * RGBA size : 4 * 4 * 4 = 64
	 * YUY2 suze : 4 * 4 * 2 = 32
	 * glReadPixels()
	 */
	glViewport(0, 0, yuy2Pairs, height);
	checkGlError("glViewport...");
//	LOGI("glUseProgram prog_id_ is %d", prog_id_);
	glUseProgram(prog_id_);
	checkGlError("glUseProgram prog_id_...");

	// Upload some uniforms if needed
	float sPerHalfTexel = (float) (0.5f / (float) width);
//	LOGI("sPerHalfTexel is %.8f...", sPerHalfTexel);
	glUniform1f(uniformLoc_sPerHalfTexel, sPerHalfTexel);
	checkGlError("glUniform1f uniformLoc_sPerHalfTexel...");

	if (IS_LUMA_UNKNOW_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_UNKNOW, (GLfloat) NV_YUY2_G_Y_UNKNOW, (GLfloat) NV_YUY2_B_Y_UNKNOW, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_UNKNOW, -(GLfloat) NV_YUY2_G_U_UNKNOW, (GLfloat) NV_YUY2_B_U_UNKNOW, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_UNKNOW, -(GLfloat) NV_YUY2_G_V_UNKNOW, -(GLfloat) NV_YUY2_B_V_UNKNOW, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coef_y_, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coef_y_");
		glUniform4fv(uniformLoc_coef_u_, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coef_u_");
		glUniform4fv(uniformLoc_coef_v_, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coef_v_");
	} else if (IS_LUMA_601_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_601, (GLfloat) NV_YUY2_G_Y_601, (GLfloat) NV_YUY2_B_Y_601, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_601, -(GLfloat) NV_YUY2_G_U_601, (GLfloat) NV_YUY2_B_U_601, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_601, -(GLfloat) NV_YUY2_G_V_601, -(GLfloat) NV_YUY2_B_V_601, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coef_y_, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coef_y_");
		glUniform4fv(uniformLoc_coef_u_, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coef_u_");
		glUniform4fv(uniformLoc_coef_v_, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coef_v_");
	} else {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_709, (GLfloat) NV_YUY2_G_Y_709, (GLfloat) NV_YUY2_B_Y_709, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_709, -(GLfloat) NV_YUY2_G_U_709, (GLfloat) NV_YUY2_B_U_709, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_709, -(GLfloat) NV_YUY2_G_V_709, -(GLfloat) NV_YUY2_B_V_709, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coef_y_, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coef_y_");
		glUniform4fv(uniformLoc_coef_u_, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coef_u_");
		glUniform4fv(uniformLoc_coef_v_, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coef_v_");
	}

	bool flipVertically = true;
	VertexCoordVec vertices[4];

	if (recording_) {
		//正常录制的参数
		vertices[0].x = -1;
		vertices[0].y = -1;
		vertices[0].s = 1;
		vertices[0].t = flipVertically ? 1 : 0;

		vertices[1].x = 1;
		vertices[1].y = -1;
		vertices[1].s = 1;
		vertices[1].t = flipVertically ? 0 : 1;

		vertices[2].x = -1;
		vertices[2].y = 1;
		vertices[2].s = 0.25;
		vertices[2].t = flipVertically ? 1 : 0;

		vertices[3].x = 1;
		vertices[3].y = 1;
		vertices[3].s = 0.25;
		vertices[3].t = flipVertically ? 0 : 1;
	} else {
		//视频合唱的参数
		vertices[0].x = -1;
		vertices[0].y = 1;
		vertices[0].s = 0;
		vertices[0].t = flipVertically ? 0 : 1;

		vertices[1].x = -1;
		vertices[1].y = -1;
		vertices[1].s = 0;
		vertices[1].t = flipVertically ? 1 : 0;

		vertices[2].x = 1;
		vertices[2].y = 1;
		vertices[2].s = 1.0;
		vertices[2].t = flipVertically ? 0 : 1;

		vertices[3].x = 1;
		vertices[3].y = -1;
		vertices[3].s = 1.0;
		vertices[3].t = flipVertically ? 1 : 0;
	}

	//
	// Draw array
	//
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glVertexAttribPointer(attrLoc_pos_, 2, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(VertexCoordVec), (const GLvoid *) &vertices[0].x);
	glEnableVertexAttribArray(attrLoc_pos_);
	checkGlError("glEnableVertexAttribArray attrLoc_pos_...");

	glVertexAttribPointer(attrLoc_texture_coord_, 2, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(VertexCoordVec), (const GLvoid *) &vertices[0].s);
	glEnableVertexAttribArray(attrLoc_texture_coord_);
	checkGlError("glEnableVertexAttribArray attrLoc_texture_coord_...");

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	checkGlError("glDrawArrays...");

	glDisableVertexAttribArray(attrLoc_pos_);
	glDisableVertexAttribArray(attrLoc_texture_coord_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void HostGPUCopier::DownloadImageFromTexture(GLuint texId, void *imageBuf, unsigned int imageWidth,
											 unsigned int imageHeight) {
	// Set texture's min/mag filter to ensure frame_ buffer_ completeness
	glBindTexture(GL_TEXTURE_2D, texId);
	checkGlError("glBindTexture texture_id_");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	checkGlError("glBindFramebuffer");
	// Attach texture to frame_ buffer_
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
	checkGlError("glFramebufferTexture2D...");

	// Download image
	glReadPixels(0, 0, (GLsizei) imageWidth, (GLsizei) imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) imageBuf);
	checkGlError("glReadPixels() failed!");

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int HostGPUCopier::GetBufferSizeInBytes(int width, int height) {
	return width * height * 2;
}

void HostGPUCopier::Destroy() {
	if (vertex_shader_)
		glDeleteShader(vertex_shader_);

	if (pixel_shader_)
		glDeleteShader(pixel_shader_);

	if(download_texture_){
		glDeleteTextures(1, &download_texture_);
	}

	if (prog_id_) {
		glDeleteProgram(prog_id_);
		prog_id_ = 0;
	}
}

bool HostGPUCopier::PrepareConvertToYUY2Program() {
	bool ret = true;
	if (prog_id_) {
		return ret;
	}
	prog_id_ = loadProgram(vertexShaderStr, convertToYUY2FragmentShaderStr);
	if (!prog_id_) {
		return false;
	}
	/**
	 * Query attribute locations and uniform locations
	 */
	attrLoc_pos_ = glGetAttribLocation(prog_id_, "posAttr");
	checkGlError("glGetAttribLocation posAttr");
	attrLoc_texture_coord_ = glGetAttribLocation(prog_id_, "texCoordAttr");
	checkGlError("glGetAttribLocation texCoordAttr");
	checkGlError("glGetAttribLocation sampler");
	uniformLoc_coef_y_ = glGetUniformLocation(prog_id_, "coefY");
	uniformLoc_coef_u_ = glGetUniformLocation(prog_id_, "coefU");
	uniformLoc_coef_v_ = glGetUniformLocation(prog_id_, "coefV");
	uniformLoc_sPerHalfTexel = glGetUniformLocation(prog_id_, "sPerHalfTexel");

	glUseProgram(prog_id_);
	/**
	 * Upload some uniforms
	 */
	if (IS_LUMA_UNKNOW_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_UNKNOW, (GLfloat) NV_YUY2_G_Y_UNKNOW, (GLfloat) NV_YUY2_B_Y_UNKNOW, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_UNKNOW, -(GLfloat) NV_YUY2_G_U_UNKNOW, (GLfloat) NV_YUY2_B_U_UNKNOW, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_UNKNOW, -(GLfloat) NV_YUY2_G_V_UNKNOW, -(GLfloat) NV_YUY2_B_V_UNKNOW, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coef_y_, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coef_y_");
		glUniform4fv(uniformLoc_coef_u_, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coef_u_");
		glUniform4fv(uniformLoc_coef_v_, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coef_v_");
	} else if (IS_LUMA_601_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_601, (GLfloat) NV_YUY2_G_Y_601, (GLfloat) NV_YUY2_B_Y_601, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_601, -(GLfloat) NV_YUY2_G_U_601, (GLfloat) NV_YUY2_B_U_601, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_601, -(GLfloat) NV_YUY2_G_V_601, -(GLfloat) NV_YUY2_B_V_601, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coef_y_, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coef_y_");
		glUniform4fv(uniformLoc_coef_u_, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coef_u_");
		glUniform4fv(uniformLoc_coef_v_, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coef_v_");
	} else {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_709, (GLfloat) NV_YUY2_G_Y_709, (GLfloat) NV_YUY2_B_Y_709, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_709, -(GLfloat) NV_YUY2_G_U_709, (GLfloat) NV_YUY2_B_U_709, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_709, -(GLfloat) NV_YUY2_G_V_709, -(GLfloat) NV_YUY2_B_V_709, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coef_y_, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coef_y_");
		glUniform4fv(uniformLoc_coef_u_, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coef_u_");
		glUniform4fv(uniformLoc_coef_v_, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coef_v_");
	}
	glUseProgram(0);

	glGenFramebuffers(1, &fbo_);
	checkGlError("glGenFramebuffers");
	return ret;
}
