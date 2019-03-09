#ifndef 	VIDEO_PLAYER_TEXTURE_FRAME_H
#define VIDEO_PLAYER_TEXTURE_FRAME_H
#include "common_tools.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/**
 * Video Texture
 */
class TextureFrame {
protected:
	bool checkGlError(const char* op);

public:
	TextureFrame();
	virtual ~TextureFrame();

	virtual bool CreateTexture() = 0;
	virtual void UpdateTexImage() = 0;
	virtual bool BindTexture(GLint *uniformSamplers) = 0;
	virtual void Dealloc() = 0;
};

#endif //VIDEO_PLAYER_TEXTURE_FRAME_H
