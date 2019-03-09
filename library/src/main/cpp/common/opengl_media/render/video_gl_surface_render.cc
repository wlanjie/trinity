#include "video_gl_surface_render.h"

#define LOG_TAG "VideoGLSurfaceRender"

VideoGLSurfaceRender::VideoGLSurfaceRender() {
	vertex_shader_ = const_cast<char *>(OUTPUT_VIEW_VERTEX_SHADER);
	fragment_shader_ = const_cast<char *>(OUTPUT_VIEW_FRAG_SHADER);
}

VideoGLSurfaceRender::~VideoGLSurfaceRender() {
}

bool VideoGLSurfaceRender::Init(int width, int height) {
	this->backing_left_ = 0;
	this->backing_top_ = 0;
	this->backing_width_ = width;
	this->backing_height_ = height;
	prog_id_ = LoadProgram(vertex_shader_, fragment_shader_);
	if (!prog_id_) {
		LOGE("Could not create program.");
		return false;
	}
	vertex_coords_ = glGetAttribLocation(prog_id_, "position_");
    CheckGlError("glGetAttribLocation vPosition");
	texture_coords_ = glGetAttribLocation(prog_id_, "texcoord");
    CheckGlError("glGetAttribLocation vTexCords");
	uniform_texture_ = glGetUniformLocation(prog_id_, "yuvTexSampler");
    CheckGlError("glGetAttribLocation yuvTexSampler");

	initialized_ = true;
	return true;
}

void VideoGLSurfaceRender::RenderToView(GLuint texID, int screenWidth, int screenHeight) {
	glViewport(0, 0, screenWidth, screenHeight);

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glUseProgram (prog_id_);
	static const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
	static const GLfloat texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
}

float VideoGLSurfaceRender::CalcCropRatio(int screenWidth, int screenHeight, int texWidth, int texHeight) {
	int fitHeight = (int)((float)texHeight*screenWidth/texWidth+0.5f);

	float ratio = (float)(fitHeight-screenHeight)/(2*fitHeight);

	return ratio;
}

// 肯定是竖屏录制
void VideoGLSurfaceRender::RenderToViewWithAutofit(GLuint texID, int screenWidth, int screenHeight, int texWidth,
                                                   int texHeight) {
	glViewport(0, 0, screenWidth, screenHeight);

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	float cropRatio = CalcCropRatio(screenWidth, screenHeight, texWidth, texHeight);

	if (cropRatio < 0)
		cropRatio = 0.0f;

	glUseProgram(prog_id_);
	static const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
			1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
	static const GLfloat texCoords[] = { 0.0f, cropRatio, 1.0f, cropRatio, 0.0f, 1.0f-cropRatio,
			1.0f, 1.0f-cropRatio };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoGLSurfaceRender::RenderToView(GLuint texID) {
	glViewport(backing_left_, backing_top_, GLsizei (backing_width_), GLsizei (backing_height_));
	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glUseProgram (prog_id_);
	static const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
//  todo: 这里有疑问，按照预想的应该是被注释掉的纹理坐标，但实际上是下面的正常工作。只有在往preview上去渲染时
//  才这样，why?
//	static const GLfloat texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoGLSurfaceRender::RenderToViewWithAutoFill(GLuint texID, int screenWidth, int screenHeight, int texWidth,
                                                    int texHeight) {
	glViewport(0, 0, screenWidth, screenHeight);

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	float textureAspectRatio = (float)texHeight / (float)texWidth;
	float viewAspectRatio = (float)screenHeight / (float)screenWidth;
	float xOffset = 0.0f;
	float yOffset = 0.0f;
	if(textureAspectRatio > viewAspectRatio){
		//Update Y Offset
		int expectedHeight = (int)((float)texHeight*screenWidth/(float)texWidth+0.5f);
		yOffset = (float)(expectedHeight-screenHeight)/(2*expectedHeight);
//		LOGI("yOffset is %.3f", yOffset);
	} else if(textureAspectRatio < viewAspectRatio){
		//Update X Offset
		int expectedWidth = (int)((float)(texHeight * screenWidth) / (float)screenHeight + 0.5);
		xOffset = (float)(texWidth - expectedWidth)/(2*texWidth);
//		LOGI("xOffset is %.3f", xOffset);
	}

	glUseProgram(prog_id_);
	static const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
    static const GLfloat texCoords[] = { xOffset, (GLfloat)(1.0 - yOffset), (GLfloat)(1.0 - xOffset), (GLfloat)(1.0 - yOffset), xOffset, yOffset,
                                         (GLfloat)(1.0 - xOffset), yOffset };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void VideoGLSurfaceRender::RenderToAutoFitTexture(GLuint inputTexId, int width, int height, GLuint outputTexId) {
	float textureAspectRatio = (float)height / (float)width;
	float viewAspectRatio = (float)backing_height_ / (float)backing_width_;
	float xOffset = 0.0f;
	float yOffset = 0.0f;
	if(textureAspectRatio > viewAspectRatio){
		//Update Y Offset
		int expectedHeight = (int)((float)height*backing_width_/(float)width+0.5f);
		yOffset = (float)(expectedHeight-backing_height_)/(2*expectedHeight);
//		LOGI("yOffset is %.3f", yOffset);
	} else if(textureAspectRatio < viewAspectRatio){
		//Update X Offset
		int expectedWidth = (int)((float)(height * backing_width_) / (float)backing_height_ + 0.5);
		xOffset = (float)(width - expectedWidth)/(2*width);
//		LOGI("xOffset is %.3f", xOffset);
	}

	glViewport(backing_left_, backing_top_, backing_width_, backing_height_);

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexId, 0);
    CheckGlError("PassThroughRender::renderEffect glFramebufferTexture2D");
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGI("failed to make complete framebuffer object %x", status);
	}

	glUseProgram(prog_id_);
	const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
    static const GLfloat texCoords[] = { xOffset, yOffset, (GLfloat)(1.0 - xOffset), yOffset, xOffset, (GLfloat)1.0 - yOffset,
                                         (GLfloat)(1.0 - xOffset), (GLfloat)(1.0 - yOffset) };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexId);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}

void VideoGLSurfaceRender::RenderToCroppedTexture(GLuint inputTexId, GLuint outputTexId, int originalWidth,
                                                  int originalHeight){
	int targetWidth = MIN(originalWidth, originalHeight);
	int targetHeight = MIN(originalWidth, originalHeight);
	int squareLength = targetHeight;
	int rectangleLength = originalWidth;
	if(originalHeight > targetWidth){
		rectangleLength = originalHeight;
	}
	float factor = (float)(rectangleLength - squareLength) / (float)rectangleLength;
	float fromYPosition = factor / 2;
	float toYPosition = 1.0 - factor / 2;
	LOGI("originalWidth is %d originalHeight is %d backing_width_ is %d backing_height_ is %d", originalWidth, originalHeight, backing_width_, backing_height_);
	glViewport(backing_left_, backing_top_, backing_width_, backing_height_);

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexId, 0);
    CheckGlError("PassThroughRender::renderEffect glFramebufferTexture2D");
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGI("failed to make complete framebuffer object %x", status);
	}

	glUseProgram (prog_id_);
    static const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
    static const GLfloat texCoords[] = { 0.0f, fromYPosition, 1.0f, fromYPosition, 0.0f, toYPosition, 1.0f, toYPosition };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexId);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}

void VideoGLSurfaceRender::RenderToEncoderTexture(GLuint inputTexId, GLuint outputTexId) {
	glViewport(backing_left_, backing_top_, GLsizei(backing_width_), GLsizei(backing_height_));

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexId, 0);
    CheckGlError("PassThroughRender::renderEffect glFramebufferTexture2D");
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGI("failed to make complete framebuffer object %x", status);
	}

	glUseProgram (prog_id_);
	static const GLfloat _vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
	static const GLfloat texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexId);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}

void VideoGLSurfaceRender::RenderToTexture(GLuint inputTexId, GLuint outputTexId) {
	glViewport(backing_left_, backing_top_, GLsizei(backing_width_), GLsizei(backing_height_));

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexId, 0);
    CheckGlError("PassThroughRender::renderEffect glFramebufferTexture2D");
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGI("failed to make complete framebuffer object %x", status);
	}

	glUseProgram (prog_id_);
	static const GLfloat _vertices[] = { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
	static const GLfloat texCoords[] = {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexId);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}

void VideoGLSurfaceRender::CheckGlError(const char *op){
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGE("after %s() glError (0x%x)\n", op, error);
	}
}

void VideoGLSurfaceRender::Destroy() {
	initialized_ = false;
	glDeleteProgram(prog_id_);
}

void VideoGLSurfaceRender::RenderToVFlipTexture(GLuint inputTexId, GLuint outputTexId) {
	glViewport(backing_left_, backing_top_, GLsizei(backing_width_), GLsizei(backing_height_));

	if (!initialized_) {
		LOGE("ViewRenderEffect::renderEffect effect not initialized!");
		return;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexId, 0);
    CheckGlError("PassThroughRender::renderEffect glFramebufferTexture2D");
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGI("failed to make complete framebuffer object %x", status);
	}

	glUseProgram (prog_id_);
	static const GLfloat _vertices[] = { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f };
	glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, 0, 0, _vertices);
	glEnableVertexAttribArray(vertex_coords_);
	static const GLfloat texCoords[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
	glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, 0, 0, texCoords);
	glEnableVertexAttribArray(texture_coords_);

	/* Binding the input texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexId);
	glUniform1i(uniform_texture_, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(vertex_coords_);
	glDisableVertexAttribArray(texture_coords_);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}

void VideoGLSurfaceRender::ResetRenderSize(int left, int top, int width, int height){
	this->backing_left_ = left;
	this->backing_top_ = top;
	this->backing_width_ = width;
	this->backing_height_ = height;
}

GLuint VideoGLSurfaceRender::LoadProgram(char *pVertexSource, char *pFragmentSource) {
	GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, pVertexSource);
	if (!vertexShader) {
		return 0;
	}
	GLuint pixelShader = LoadShader(GL_FRAGMENT_SHADER, pFragmentSource);
	if (!pixelShader) {
		return 0;
	}
	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexShader);
        CheckGlError("glAttachShader");
		glAttachShader(program, pixelShader);
        CheckGlError("glAttachShader");
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

GLuint VideoGLSurfaceRender::LoadShader(GLenum shaderType, const char *pSource) {
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
