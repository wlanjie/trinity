#include "audio_process_encoder_adapter.h"

#define LOG_TAG "AudioProcessEncoderAdapter"

AudioProcessEncoderAdapter::AudioProcessEncoderAdapter() {
	musicMerger = NULL;
	accompanyPacketPool = NULL;
}

AudioProcessEncoderAdapter::~AudioProcessEncoderAdapter() {
}

void AudioProcessEncoderAdapter::init(LivePacketPool* pcmPacketPool, int audioSampleRate, int audioChannels,
		int audioBitRate, const char* audio_codec_name) {
	this->accompanyPacketPool = LiveCommonPacketPool::GetInstance();
	musicMerger = new MusicMerger();
	musicMerger->initWithAudioEffectProcessor(audioSampleRate);
	AudioEncoderAdapter::init(pcmPacketPool, audioSampleRate, audioChannels, audioBitRate, audio_codec_name);
	this->channelRatio = 2.0f;
}

int AudioProcessEncoderAdapter::processAudio() {
	int ret = packetBufferSize;
	//伴奏
	LiveAudioPacket *accompanyPacket = NULL;
	if (accompanyPacketPool->getAccompanyPacket(&accompanyPacket, true) < 0) {
		return -1;
	}
	if (NULL != accompanyPacket) {
		int accompanySampleSize = accompanyPacket->size;
		short* accompanySamples = accompanyPacket->buffer;
		long frameNum = accompanyPacket->frameNum;
		ret = musicMerger->mixtureMusicBuffer(frameNum, accompanySamples, accompanySampleSize, packetBuffer, packetBufferSize);
		delete accompanyPacket;
		accompanyPacket = NULL;
	}
	return ret;
}

void AudioProcessEncoderAdapter::destroy(){
	accompanyPacketPool->abortAccompanyPacketQueue();
	AudioEncoderAdapter::destroy();
	accompanyPacketPool->destoryAccompanyPacketQueue();
	if (NULL != musicMerger) {
		musicMerger->stop();
		delete musicMerger;
		musicMerger = NULL;
	}
}

void AudioProcessEncoderAdapter::discardAudioPacket() {
	AudioEncoderAdapter::discardAudioPacket();
	while (accompanyPacketPool->detectDiscardAccompanyPacket()) {
        if(!accompanyPacketPool->discardAccompanyPacket()){
            break;
        }
	}
}
