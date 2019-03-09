#include "video_encoder_adapter.h"

#define LOG_TAG "VideoEncoderAdapter"

VideoEncoderAdapter::VideoEncoderAdapter() {
	renderer_ = NULL;
}

VideoEncoderAdapter::~VideoEncoderAdapter() {
}

void VideoEncoderAdapter::init(int width, int height, int videoBitRate, float frameRate){
	packet_pool_ = LiveCommonPacketPool::GetInstance();
	this->video_width_ = width;
	this->video_height_ = height;
	this->frameRate = frameRate;
	this->video_bit_rate_ = videoBitRate;
	this->encoded_frame_count_ = 0;

	LOGI("Init(width:%d, height:%d, video_bit_rate_:%d, frame_rate_:%d)", width, height, videoBitRate, frameRate);
}

void VideoEncoderAdapter::ResetFpsStartTimeIfNeed(int fps){
	 if (fabs(fps - this->frameRate) > FLOAT_DELTA){
		 this->frameRate = fps;
		 LOGI("HotConfig reset resetFpsStartTime");
		 encoded_frame_count_ = 0;
		 fps_change_time_ = getCurrentTime();
	 }
}
