#include "video_encoder_adapter.h"

#define LOG_TAG "VideoEncoderAdapter"

VideoEncoderAdapter::VideoEncoderAdapter() {
	renderer = NULL;
}

VideoEncoderAdapter::~VideoEncoderAdapter() {
}

void VideoEncoderAdapter::init(int width, int height, int videoBitRate, float frameRate){
	packetPool = LiveCommonPacketPool::GetInstance();
	this->videoWidth = width;
	this->videoHeight = height;
	this->frameRate = frameRate;
	this->videoBitRate = videoBitRate;
	this->encodedFrameCount = 0;

	LOGI("Init(width:%d, height:%d, videoBitRate:%d, frameRate:%d)", width, height, videoBitRate, frameRate);
}

void VideoEncoderAdapter::resetFpsStartTimeIfNeed(int fps){
	 if (fabs(fps - this->frameRate) > FLOAT_DELTA){
		 this->frameRate = fps;
		 LOGI("HotConfig reset resetFpsStartTime");
		 encodedFrameCount = 0;
		 fpsChangeTime = getCurrentTime();
	 }
}
