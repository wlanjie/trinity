#include "texture_frame_copier.h"

#define LOG_TAG "TextureFrameCopier"

TextureFrameCopier::TextureFrameCopier() {
}

TextureFrameCopier::~TextureFrameCopier() {
}

void TextureFrameCopier::Destroy() {
	initialized_ = false;
	if (prog_id_) {
		glDeleteProgram(prog_id_);
		prog_id_ = 0;
	}
}
