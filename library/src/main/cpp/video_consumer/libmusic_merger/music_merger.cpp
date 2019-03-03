#include "music_merger.h"

#define LOG_TAG "SongStudio MusicMerger"

MusicMerger::MusicMerger() {
	audioSampleRate = -1;
	frameNum = 0;
}

MusicMerger::~MusicMerger() {

}

void MusicMerger::initWithAudioEffectProcessor(int audioSampleRate) {
	this->audioSampleRate = audioSampleRate;
}

/** 在直播中调用 **/
int MusicMerger::mixtureMusicBuffer(long frameNum, short* accompanySamples, int accompanySampleSize, short* audioSamples, int audioSampleSize) {
	for(int i = audioSampleSize - 1; i >= 0; i--) {
		audioSamples[i] = audioSamples[i / 2];
	}
	int actualSize = MIN(accompanySampleSize, audioSampleSize);
	LOGI("accompanySampleSize is %d audioSampleSize is %d", accompanySampleSize, audioSampleSize);
	mixtureAccompanyAudio(accompanySamples, audioSamples, actualSize, audioSamples);
	return actualSize;
}

void MusicMerger::stop() {
}
