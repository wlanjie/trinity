#include "audio_process_encoder_adapter.h"

#define LOG_TAG "AudioProcessEncoderAdapter"

AudioProcessEncoderAdapter::AudioProcessEncoderAdapter() {
	musicMerger = NULL;
	accompanyPacketPool = NULL;
}

AudioProcessEncoderAdapter::~AudioProcessEncoderAdapter() {
}

void AudioProcessEncoderAdapter::Init(LivePacketPool *pcmPacketPool, int audioSampleRate, int audioChannels,
                                      int audioBitRate, const char *audio_codec_name) {
	this->accompanyPacketPool = LiveCommonPacketPool::GetInstance();
	musicMerger = new MusicMerger();
	musicMerger->initWithAudioEffectProcessor(audioSampleRate);
    AudioEncoderAdapter::Init(pcmPacketPool, audioSampleRate, audioChannels, audioBitRate, audio_codec_name);
	this->channel_ratio_ = 2.0f;
}

int AudioProcessEncoderAdapter::ProcessAudio() {
	int ret = packet_buffer_size_;
	//伴奏
	LiveAudioPacket *accompanyPacket = NULL;
	if (accompanyPacketPool->getAccompanyPacket(&accompanyPacket, true) < 0) {
		return -1;
	}
	if (NULL != accompanyPacket) {
		int accompanySampleSize = accompanyPacket->size;
		short* accompanySamples = accompanyPacket->buffer;
		long frameNum = accompanyPacket->frameNum;
		ret = musicMerger->mixtureMusicBuffer(frameNum, accompanySamples, accompanySampleSize, packet_buffer_, packet_buffer_size_);
		delete accompanyPacket;
		accompanyPacket = NULL;
	}
	return ret;
}

void AudioProcessEncoderAdapter::Destroy(){
	accompanyPacketPool->abortAccompanyPacketQueue();
	AudioEncoderAdapter::Destroy();
	accompanyPacketPool->destoryAccompanyPacketQueue();
	if (NULL != musicMerger) {
		musicMerger->stop();
		delete musicMerger;
		musicMerger = NULL;
	}
}

void AudioProcessEncoderAdapter::DiscardAudioPacket() {
	AudioEncoderAdapter::DiscardAudioPacket();
	while (accompanyPacketPool->detectDiscardAccompanyPacket()) {
        if(!accompanyPacketPool->discardAccompanyPacket()){
            break;
        }
	}
}
