#include "audio_encoder_adapter.h"

#define LOG_TAG "AudioEncoderAdapter"

AudioEncoderAdapter::AudioEncoderAdapter() {
	audioCodecName = NULL;
	audioEncoder = NULL;
	isEncoding = false;
}

AudioEncoderAdapter::~AudioEncoderAdapter() {
}

static int fill_pcm_frame_callback(int16_t *samples, int frame_size, int nb_channels, double* presentationTimeMills, void *context) {
	AudioEncoderAdapter* audioEncoderAdapter = (AudioEncoderAdapter*) context;
	return audioEncoderAdapter->getAudioFrame(samples, frame_size, nb_channels, presentationTimeMills);
}

void AudioEncoderAdapter::init(LivePacketPool* pcmPacketPool, int audioSampleRate,
		int audioChannels, int audioBitRate, const char* audio_codec_name){
	/** iOS是1.0f Android是2.0f **/
	this->channelRatio = 1.0f;
	this->packetBuffer = NULL;
	this->packetBufferSize = 0;
	this->packetBufferCursor = 0;
	this->pcmPacketPool = pcmPacketPool;
	this->audioSampleRate = audioSampleRate;
	this->audioChannels = audioChannels;
	this->audioBitRate = audioBitRate;
	int audioCodecNameLength = strlen(audio_codec_name);
	audioCodecName = new char[audioCodecNameLength + 1];
	memset(audioCodecName, 0, audioCodecNameLength + 1);
	memcpy(audioCodecName, audio_codec_name, audioCodecNameLength);
	this->isEncoding = true;
	this->aacPacketPool = LiveAudioPacketPool::GetInstance();
	pthread_create(&audioEncoderThread, NULL, startEncodeThread, this);
}

void* AudioEncoderAdapter::startEncodeThread(void* ptr) {
	AudioEncoderAdapter* audioEncoderAdapter = (AudioEncoderAdapter *) ptr;
	audioEncoderAdapter->startEncode();
	pthread_exit(0);
	return 0;
}

void AudioEncoderAdapter::startEncode(){
	audioEncoder = new AudioEncoder();
	audioEncoder->init(audioBitRate, audioChannels, audioSampleRate, audioCodecName,
			fill_pcm_frame_callback, this);
	while(isEncoding){
		//1:调用AudioEncoder进行编码并且打上时间戳
		LiveAudioPacket *audioPacket = NULL;
		int ret = audioEncoder->encode(&audioPacket);
		//2:将编码之后的数据放入aacPacketPool中
		if(ret >= 0 && NULL != audioPacket){
			aacPacketPool->pushAudioPacketToQueue(audioPacket);
		}
	}
}

void AudioEncoderAdapter::destroy(){
	isEncoding = false;
	pcmPacketPool->abortAudioPacketQueue();
	pthread_join(audioEncoderThread, 0);
	pcmPacketPool->destoryAudioPacketQueue();
	if (NULL != audioEncoder) {
		audioEncoder->destroy();
		delete audioEncoder;
		audioEncoder = NULL;
	}
	if(NULL != audioCodecName){
		delete[] audioCodecName;
		audioCodecName = NULL;
	}
	if(NULL != packetBuffer){
		delete packetBuffer;
		packetBuffer = NULL;
	}
}

int AudioEncoderAdapter::getAudioFrame(int16_t * samples, int frame_size, int nb_channels,
		double* presentationTimeMills) {
    int byteSize = frame_size * nb_channels * 2;
    int samplesInShortCursor = 0;
    while (true) {
        if (packetBufferSize == 0) {
            int ret = this->getAudioPacket();
            if (ret < 0) {
                return ret;
            }
        }
        int copyToSamplesInShortSize = (byteSize - samplesInShortCursor * 2) / 2;
        if (packetBufferCursor + copyToSamplesInShortSize <= packetBufferSize) {
            this->cpyToSamples(samples, samplesInShortCursor, copyToSamplesInShortSize, presentationTimeMills);
            packetBufferCursor += copyToSamplesInShortSize;
            samplesInShortCursor = 0;
            break;
        } else {
            int subPacketBufferSize = packetBufferSize - packetBufferCursor;
            this->cpyToSamples(samples, samplesInShortCursor, subPacketBufferSize, presentationTimeMills);
            samplesInShortCursor += subPacketBufferSize;
            packetBufferSize = 0;
            continue;
        }
    }
    return frame_size * nb_channels;
}

int AudioEncoderAdapter::cpyToSamples(int16_t * samples, int samplesInShortCursor, int cpyPacketBufferSize, double* presentationTimeMills) {
	if(0 == samplesInShortCursor){
		//第一次为samples拷贝进sample 这里要记录时间给sampleBuffer
		double packetBufferCursorDuration = (double)packetBufferCursor * 1000.0f / (double)(audioSampleRate * channelRatio);
		(*presentationTimeMills) = packetBufferPresentationTimeMills + packetBufferCursorDuration;
	}
    memcpy(samples + samplesInShortCursor, packetBuffer + packetBufferCursor, cpyPacketBufferSize * sizeof(short));
    return 1;
}

void AudioEncoderAdapter::discardAudioPacket() {
    while(pcmPacketPool->detectDiscardAudioPacket()){
        if(!pcmPacketPool->discardAudioPacket()){
            break;
        }
    }
}

int AudioEncoderAdapter::getAudioPacket() {
	this->discardAudioPacket();
	LiveAudioPacket *audioPacket = NULL;
	if (pcmPacketPool->getAudioPacket(&audioPacket, true) < 0) {
		return -1;
	}
	packetBufferCursor = 0;
	packetBufferPresentationTimeMills = audioPacket->position;
	/**
	 * 在Android平台 录制是单声道的 经过音效处理之后是双声道 channelRatio 2
	 * 在iOS平台 录制的是双声道的 是已经处理音效过后的 channelRatio 1
	 */
	packetBufferSize = audioPacket->size * channelRatio;
	if (NULL == packetBuffer) {
		packetBuffer = new short[packetBufferSize];
	}
	memcpy(packetBuffer, audioPacket->buffer, audioPacket->size * sizeof(short));
	int actualSize = this->processAudio();
	if (actualSize > 0 && actualSize < packetBufferSize) {
		packetBufferCursor = packetBufferSize - actualSize;
		memmove(packetBuffer + packetBufferCursor, packetBuffer, actualSize * sizeof(short));
	}
	if (NULL != audioPacket) {
		delete audioPacket;
		audioPacket = NULL;
	}
	return actualSize > 0 ? 1 : -1;
}
