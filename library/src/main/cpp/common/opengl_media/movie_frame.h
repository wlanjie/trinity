#ifndef VIDEO_PLAYER_MOVIE_FRAME_H
#define VIDEO_PLAYER_MOVIE_FRAME_H

#include "CommonTools.h"

/** 为了避免类文件过多，现在把有关数据结构的model都写到了这个文件中 **/
typedef enum {
    MovieFrameTypeNone,
    MovieFrameTypeAudio,
    MovieFrameTypeVideo
} MovieFrameType;

class MovieFrame{
public:
	float position;
	float duration;
	MovieFrame();
	virtual MovieFrameType getType() = 0;
};
class AudioFrame: public MovieFrame{
public:
	byte * samples;
	int size;
	AudioFrame();
	~AudioFrame();
	MovieFrameType getType(){
		return MovieFrameTypeAudio;
	};
	bool dataUseUp;
	void fillFullData();
	void useUpData();
	bool isDataUseUp();
};

class VideoFrame: public MovieFrame{
public:
	uint8_t * luma;
	uint8_t * chromaB;
	uint8_t * chromaR;
	int width;
	int height;
	VideoFrame();
	~VideoFrame();

	VideoFrame* clone(){
		VideoFrame* frame = new VideoFrame();
		frame->width = width;
		frame->height = height;
		int lumaLength = width * height;
		frame->luma = new uint8_t[lumaLength];
		memcpy(frame->luma, luma, lumaLength);
		int chromaBLength = width * height / 4;
		frame->chromaB = new uint8_t[chromaBLength];
		memcpy(frame->chromaB, chromaB, chromaBLength);
		int chromaRLength = chromaBLength;
		frame->chromaR = new uint8_t[chromaRLength];
		memcpy(frame->chromaR, chromaR, chromaRLength);
		return frame;
	}

	MovieFrameType getType(){
		return MovieFrameTypeVideo;
	};
};

#endif //VIDEO_PLAYER_MOVIE_FRAME_H

