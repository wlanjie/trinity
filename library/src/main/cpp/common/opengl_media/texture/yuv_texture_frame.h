#ifndef 	VIDEO_PLAYER_YUV_TEXTURE_FRAME_H
#define VIDEO_PLAYER_YUV_TEXTURE_FRAME_H

#include "opengl_media/movie_frame.h"
#include "texture_frame.h"

/**
 * Video Host Texture
 */
class YUVTextureFrame: public TextureFrame {
private:
	bool writeFlag;
	GLuint textures[3];
	int initTexture();

	VideoFrame *frame;
public:
	YUVTextureFrame();
	virtual ~YUVTextureFrame();

	void setVideoFrame(VideoFrame *yuvFrame);

	bool createTexture();
	void updateTexImage();
	bool bindTexture(GLint* uniformSamplers);
	void dealloc();
};

#endif //VIDEO_PLAYER_YUV_TEXTURE_FRAME_H
