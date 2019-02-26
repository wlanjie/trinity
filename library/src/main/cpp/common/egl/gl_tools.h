#ifndef CHANGBA_STUDIO_OPENGL_TOOLS_H_
#define CHANGBA_STUDIO_OPENGL_TOOLS_H_
#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "../../log.h"

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
				LOGI( "Guessing at GL_INFO_LOG_LENGTH size\n");
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

static inline GLuint loadProgram(char* pVertexSource, char* pFragmentSource) {
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
#endif //CHANGBA_STUDIO_OPENGL_TOOLS_H_
