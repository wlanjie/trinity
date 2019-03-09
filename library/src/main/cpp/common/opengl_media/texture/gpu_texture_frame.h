#ifndef VIDEO_PLAYER_GPU_TEXTURE_FRAME_H
#define VIDEO_PLAYER_GPU_TEXTURE_FRAME_H

#include "texture_frame.h"

class GPUTextureFrame: public TextureFrame {
private:
	GLuint decode_texture_id_;
	int InitTexture();

public:
	GPUTextureFrame();
	virtual ~GPUTextureFrame();

	bool CreateTexture();
	void UpdateTexImage();
	bool BindTexture(GLint *uniformSamplers);
	void Dealloc();

	GLuint GetDecodeTexId() {
		return decode_texture_id_;
	};

};

#endif //VIDEO_PLAYER_GPU_TEXTURE_FRAME_H
