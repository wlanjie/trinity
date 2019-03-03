#include "texture_frame_copier.h"

#define LOG_TAG "TextureFrameCopier"

TextureFrameCopier::TextureFrameCopier() {
}

TextureFrameCopier::~TextureFrameCopier() {
}

void TextureFrameCopier::destroy() {
	mIsInitialized = false;
	if (mGLProgId) {
		glDeleteProgram(mGLProgId);
		mGLProgId = 0;
	}
}
