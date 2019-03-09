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
	int InitTexture();

	VideoFrame *frame_;
public:
	YUVTextureFrame();
	virtual ~YUVTextureFrame();

	void SetVideoFrame(VideoFrame *yuvFrame);

	bool CreateTexture();
	void UpdateTexImage();
	bool BindTexture(GLint *uniformSamplers);
	void Dealloc();
};

#endif //VIDEO_PLAYER_YUV_TEXTURE_FRAME_H
