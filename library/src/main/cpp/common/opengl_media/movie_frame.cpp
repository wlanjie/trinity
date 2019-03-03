#include "movie_frame.h"

#define LOG_TAG "MovieFrame"

MovieFrame::MovieFrame() {
	position = 0.0f;
	duration = 0.0f;
}

AudioFrame::AudioFrame() {
	dataUseUp = true;
	samples = NULL;
	size = 0;
}
void AudioFrame::fillFullData() {
	dataUseUp = false;
}
void AudioFrame::useUpData() {
	dataUseUp = true;
}
bool AudioFrame::isDataUseUp() {
	return dataUseUp;
}
AudioFrame::~AudioFrame() {
	if (NULL != samples) {
		delete[] samples;
		samples = NULL;
	}
}

VideoFrame::VideoFrame() {
	luma = NULL;
	chromaB = NULL;
	chromaR = NULL;
	width = 0;
	height = 0;
}

VideoFrame::~VideoFrame() {
	if (NULL != luma) {
		delete[] luma;
		luma = NULL;
	}
	if (NULL != chromaB) {
		delete[] chromaB;
		chromaB = NULL;
	}
	if (NULL != chromaR) {
		delete[] chromaR;
		chromaR = NULL;
	}
}
