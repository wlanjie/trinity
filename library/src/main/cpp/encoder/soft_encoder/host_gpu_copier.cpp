#include "./host_gpu_copier.h"

#define LOG_TAG "HostGPUCopier"

HostGPUCopier::HostGPUCopier() {
	mGLProgId = 0;
	downloadTex = 0;
	this->isRecording = false;
}

HostGPUCopier::~HostGPUCopier() {
}

void HostGPUCopier::copyYUY2Image(GLuint ipTex, byte* yuy2ImageBuffer, int width, int height) {
	//将纹理渲染到YUY2格式的数据
//	LOGI("width is %d height is %d", width, height);
	this->convertYUY2(ipTex, width, height);
	//Download image
	const unsigned int yuy2Pairs = (width + 1) / 2;
	int imageBufferSize = getBufferSizeInBytes(width, height);
	this->downloadImageFromTexture(downloadTex, yuy2ImageBuffer, yuy2Pairs, height);
}

void HostGPUCopier::ensureTextureStorage(GLint internalFormat, const unsigned int yuy2Pairs, int height) {
	if (!downloadTex) {
		glGenTextures(1, &downloadTex);
		checkGlError("glGenTextures downloadTex");
		glBindTexture(GL_TEXTURE_2D, downloadTex);
		checkGlError("glBindTexture downloadTex");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glBindTexture(GL_TEXTURE_2D, downloadTex);
		checkGlError("glBindTexture downloadTex");
	}
	// Specify texture
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, yuy2Pairs, (GLsizei) height, 0, internalFormat, GL_UNSIGNED_BYTE, 0);
}

void HostGPUCopier::convertYUY2(GLuint ipTex, int width, int height) {
	prepareConvertToYUY2Program();

	const unsigned int yuy2Pairs = (width + 1) / 2;
	//GL_OES_required_internalFormat  0x8058 // GL_RGBA8/GL_RGBA8_OES
	GLint internalFormat = GL_RGBA;
	ensureTextureStorage(internalFormat, yuy2Pairs, height);

	glActiveTexture(GL_TEXTURE0);
	checkGlError("glActiveTexture GL_TEXTURE0");
	glBindTexture(GL_TEXTURE_2D, ipTex);
	checkGlError("glBindTexture ipTex");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Attach YUY2 texture to frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	checkGlError("glBindFramebuffer");
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, downloadTex, 0);
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
//	LOGI("glUseProgram mGLProgId is %d", mGLProgId);
	glUseProgram(mGLProgId);
	checkGlError("glUseProgram mGLProgId...");

	// Upload some uniforms if needed
	float sPerHalfTexel = (float) (0.5f / (float) width);
//	LOGI("sPerHalfTexel is %.8f...", sPerHalfTexel);
	glUniform1f(uniformLoc_sPerHalfTexel, sPerHalfTexel);
	checkGlError("glUniform1f uniformLoc_sPerHalfTexel...");

	if (IS_LUMA_UNKNOW_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_UNKNOW, (GLfloat) NV_YUY2_G_Y_UNKNOW, (GLfloat) NV_YUY2_B_Y_UNKNOW, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_UNKNOW, -(GLfloat) NV_YUY2_G_U_UNKNOW, (GLfloat) NV_YUY2_B_U_UNKNOW, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_UNKNOW, -(GLfloat) NV_YUY2_G_V_UNKNOW, -(GLfloat) NV_YUY2_B_V_UNKNOW, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coefY, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coefY");
		glUniform4fv(uniformLoc_coefU, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coefU");
		glUniform4fv(uniformLoc_coefV, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coefV");
	} else if (IS_LUMA_601_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_601, (GLfloat) NV_YUY2_G_Y_601, (GLfloat) NV_YUY2_B_Y_601, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_601, -(GLfloat) NV_YUY2_G_U_601, (GLfloat) NV_YUY2_B_U_601, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_601, -(GLfloat) NV_YUY2_G_V_601, -(GLfloat) NV_YUY2_B_V_601, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coefY, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coefY");
		glUniform4fv(uniformLoc_coefU, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coefU");
		glUniform4fv(uniformLoc_coefV, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coefV");
	} else {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_709, (GLfloat) NV_YUY2_G_Y_709, (GLfloat) NV_YUY2_B_Y_709, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_709, -(GLfloat) NV_YUY2_G_U_709, (GLfloat) NV_YUY2_B_U_709, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_709, -(GLfloat) NV_YUY2_G_V_709, -(GLfloat) NV_YUY2_B_V_709, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coefY, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coefY");
		glUniform4fv(uniformLoc_coefU, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coefU");
		glUniform4fv(uniformLoc_coefV, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coefV");
	}

	bool flipVertically = true;
	VertexCoordVec vertices[4];

	if (isRecording) {
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

	glVertexAttribPointer(attrLoc_pos, 2, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(VertexCoordVec), (const GLvoid *) &vertices[0].x);
	glEnableVertexAttribArray(attrLoc_pos);
	checkGlError("glEnableVertexAttribArray attrLoc_pos...");

	glVertexAttribPointer(attrLoc_texCoord, 2, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(VertexCoordVec), (const GLvoid *) &vertices[0].s);
	glEnableVertexAttribArray(attrLoc_texCoord);
	checkGlError("glEnableVertexAttribArray attrLoc_texCoord...");

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	checkGlError("glDrawArrays...");

	glDisableVertexAttribArray(attrLoc_pos);
	glDisableVertexAttribArray(attrLoc_texCoord);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void HostGPUCopier::downloadImageFromTexture(GLuint texId, void *imageBuf, unsigned int imageWidth, unsigned int imageHeight) {
	// Set texture's min/mag filter to ensure frame buffer completeness
	glBindTexture(GL_TEXTURE_2D, texId);
	checkGlError("glBindTexture texId");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	checkGlError("glBindFramebuffer");
	// Attach texture to frame buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
	checkGlError("glFramebufferTexture2D...");

	// Download image
	glReadPixels(0, 0, (GLsizei) imageWidth, (GLsizei) imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) imageBuf);
	checkGlError("glReadPixels() failed!");

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int HostGPUCopier::getBufferSizeInBytes(int width, int height) {
	return width * height * 2;
}

void HostGPUCopier::destroy() {
	if (vertexShader)
		glDeleteShader(vertexShader);

	if (pixelShader)
		glDeleteShader(pixelShader);

	if(downloadTex){
		glDeleteTextures(1, &downloadTex);
	}

	if (mGLProgId) {
		glDeleteProgram(mGLProgId);
		mGLProgId = 0;
	}
}

bool HostGPUCopier::prepareConvertToYUY2Program() {
	bool ret = true;
	if (mGLProgId) {
		return ret;
	}
	mGLProgId = loadProgram(vertexShaderStr, convertToYUY2FragmentShaderStr);
	if (!mGLProgId) {
		return false;
	}
	/**
	 * Query attribute locations and uniform locations
	 */
	attrLoc_pos = glGetAttribLocation(mGLProgId, "posAttr");
	checkGlError("glGetAttribLocation posAttr");
	attrLoc_texCoord = glGetAttribLocation(mGLProgId, "texCoordAttr");
	checkGlError("glGetAttribLocation texCoordAttr");
	mGLUniformTexture = glGetUniformLocation(mGLProgId, "sampler");
	checkGlError("glGetAttribLocation sampler");
	uniformLoc_coefY = glGetUniformLocation(mGLProgId, "coefY");
	uniformLoc_coefU = glGetUniformLocation(mGLProgId, "coefU");
	uniformLoc_coefV = glGetUniformLocation(mGLProgId, "coefV");
	uniformLoc_sPerHalfTexel = glGetUniformLocation(mGLProgId, "sPerHalfTexel");

	glUseProgram(mGLProgId);
	/**
	 * Upload some uniforms
	 */
	if (IS_LUMA_UNKNOW_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_UNKNOW, (GLfloat) NV_YUY2_G_Y_UNKNOW, (GLfloat) NV_YUY2_B_Y_UNKNOW, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_UNKNOW, -(GLfloat) NV_YUY2_G_U_UNKNOW, (GLfloat) NV_YUY2_B_U_UNKNOW, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_UNKNOW, -(GLfloat) NV_YUY2_G_V_UNKNOW, -(GLfloat) NV_YUY2_B_V_UNKNOW, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coefY, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coefY");
		glUniform4fv(uniformLoc_coefU, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coefU");
		glUniform4fv(uniformLoc_coefV, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coefV");
	} else if (IS_LUMA_601_FLAG) {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_601, (GLfloat) NV_YUY2_G_Y_601, (GLfloat) NV_YUY2_B_Y_601, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_601, -(GLfloat) NV_YUY2_G_U_601, (GLfloat) NV_YUY2_B_U_601, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_601, -(GLfloat) NV_YUY2_G_V_601, -(GLfloat) NV_YUY2_B_V_601, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coefY, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coefY");
		glUniform4fv(uniformLoc_coefU, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coefU");
		glUniform4fv(uniformLoc_coefV, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coefV");
	} else {
		GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_709, (GLfloat) NV_YUY2_G_Y_709, (GLfloat) NV_YUY2_B_Y_709, (GLfloat) 16 / 255 };
		GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_709, -(GLfloat) NV_YUY2_G_U_709, (GLfloat) NV_YUY2_B_U_709, (GLfloat) 128 / 255 };
		GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_709, -(GLfloat) NV_YUY2_G_V_709, -(GLfloat) NV_YUY2_B_V_709, (GLfloat) 128 / 255 };

		glUniform4fv(uniformLoc_coefY, 1, coefYVec);
		checkGlError("glUniform4fv uniformLoc_coefY");
		glUniform4fv(uniformLoc_coefU, 1, coefUVec);
		checkGlError("glUniform4fv uniformLoc_coefU");
		glUniform4fv(uniformLoc_coefV, 1, coefVVec);
		checkGlError("glUniform4fv uniformLoc_coefV");
	}
	glUseProgram(0);

	glGenFramebuffers(1, &FBO);
	checkGlError("glGenFramebuffers");
	return ret;
}
