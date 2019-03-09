#include "texture_frame.h"

#define LOG_TAG "TextureFrame"

TextureFrame::TextureFrame() {

}

TextureFrame::~TextureFrame() {

}

bool TextureFrame::checkGlError(const char* op) {
	GLint error;
	for (error = glGetError(); error; error = glGetError()) {
		LOGI("error::after %s() glError (0x%x)\n", op, error);
		return true;
	}
	return false;
}
