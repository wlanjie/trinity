#ifndef 	VIDEO_PLAYER_TEXTURE_FRAME_H
#define VIDEO_PLAYER_TEXTURE_FRAME_H
#include "CommonTools.h"
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

	virtual bool createTexture() = 0;
	virtual void updateTexImage() = 0;
	virtual bool bindTexture(GLint* uniformSamplers) = 0;
	virtual void dealloc() = 0;
};

#endif //VIDEO_PLAYER_TEXTURE_FRAME_H
