#include "audio_encoder_adapter.h"

#define LOG_TAG "AudioEncoderAdapter"

AudioEncoderAdapter::AudioEncoderAdapter() {
	audio_codec_name_ = NULL;
	audio_encoder_ = NULL;
	encoding_ = false;
}

AudioEncoderAdapter::~AudioEncoderAdapter() {
}

static int FillPCMFrameCallback(int16_t *samples, int frame_size, int nb_channels, double *presentationTimeMills,
								void *context) {
	AudioEncoderAdapter* audioEncoderAdapter = (AudioEncoderAdapter*) context;
	return audioEncoderAdapter->GetAudioFrame(samples, frame_size, nb_channels, presentationTimeMills);
}

void AudioEncoderAdapter::Init(LivePacketPool *pcmPacketPool, int audioSampleRate,
							   int audioChannels, int audioBitRate, const char *audio_codec_name){
	/** iOS是1.0f Android是2.0f **/
	this->channel_ratio_ = 1.0f;
	this->packet_buffer_ = NULL;
	this->packet_buffer_size_ = 0;
	this->packet_buffer_cursor_ = 0;
	this->pcm_packet_pool_ = pcmPacketPool;
	this->audio_sample_rate_ = audioSampleRate;
	this->audio_channels_ = audioChannels;
	this->audio_bit_rate_ = audioBitRate;
	int audioCodecNameLength = strlen(audio_codec_name);
	audio_codec_name_ = new char[audioCodecNameLength + 1];
	memset(audio_codec_name_, 0, audioCodecNameLength + 1);
	memcpy(audio_codec_name_, audio_codec_name, audioCodecNameLength);
	this->encoding_ = true;
	this->aac_packet_pool_ = LiveAudioPacketPool::GetInstance();
	pthread_create(&audio_encoder_thread_, NULL, StartEncodeThread, this);
}

void* AudioEncoderAdapter::StartEncodeThread(void *ptr) {
	AudioEncoderAdapter* audioEncoderAdapter = (AudioEncoderAdapter *) ptr;
	audioEncoderAdapter->StartEncode();
	pthread_exit(0);
	return 0;
}

void AudioEncoderAdapter::StartEncode(){
	audio_encoder_ = new AudioEncoder();
	audio_encoder_->Init(audio_bit_rate_, audio_channels_, audio_sample_rate_, audio_codec_name_,
					   FillPCMFrameCallback, this);
	while(encoding_){
		//1:调用AudioEncoder进行编码并且打上时间戳
		LiveAudioPacket *audioPacket = NULL;
		int ret = audio_encoder_->Encode(&audioPacket);
		//2:将编码之后的数据放入aacPacketPool中
		if(ret >= 0 && NULL != audioPacket){
			aac_packet_pool_->PushAudioPacketToQueue(audioPacket);
		}
	}
}

void AudioEncoderAdapter::Destroy(){
	encoding_ = false;
	pcm_packet_pool_->AbortAudioPacketQueue();
	pthread_join(audio_encoder_thread_, 0);
	pcm_packet_pool_->DestroyAudioPacketQueue();
	if (NULL != audio_encoder_) {
		audio_encoder_->Destroy();
		delete audio_encoder_;
		audio_encoder_ = NULL;
	}
	if(NULL != audio_codec_name_){
		delete[] audio_codec_name_;
		audio_codec_name_ = NULL;
	}
	if(NULL != packet_buffer_){
		delete packet_buffer_;
		packet_buffer_ = NULL;
	}
}

int AudioEncoderAdapter::GetAudioFrame(int16_t *samples, int frame_size, int nb_channels,
									   double *presentationTimeMills) {
    int byteSize = frame_size * nb_channels * 2;
    int samplesInShortCursor = 0;
    while (true) {
        if (packet_buffer_size_ == 0) {
            int ret = this->GetAudioPacket();
            if (ret < 0) {
                return ret;
            }
        }
        int copyToSamplesInShortSize = (byteSize - samplesInShortCursor * 2) / 2;
        if (packet_buffer_cursor_ + copyToSamplesInShortSize <= packet_buffer_size_) {
			this->CpyToSamples(samples, samplesInShortCursor, copyToSamplesInShortSize, presentationTimeMills);
            packet_buffer_cursor_ += copyToSamplesInShortSize;
            samplesInShortCursor = 0;
            break;
        } else {
            int subPacketBufferSize = packet_buffer_size_ - packet_buffer_cursor_;
			this->CpyToSamples(samples, samplesInShortCursor, subPacketBufferSize, presentationTimeMills);
            samplesInShortCursor += subPacketBufferSize;
            packet_buffer_size_ = 0;
            continue;
        }
    }
    return frame_size * nb_channels;
}

int AudioEncoderAdapter::CpyToSamples(int16_t *samples, int samplesInShortCursor, int cpyPacketBufferSize,
									  double *presentationTimeMills) {
	if(0 == samplesInShortCursor){
		//第一次为samples拷贝进sample 这里要记录时间给sampleBuffer
		double packetBufferCursorDuration = (double)packet_buffer_cursor_ * 1000.0f / (double)(audio_sample_rate_ * channel_ratio_);
		(*presentationTimeMills) = packet_buffer_presentation_time_mills_ + packetBufferCursorDuration;
	}
    memcpy(samples + samplesInShortCursor, packet_buffer_ + packet_buffer_cursor_, cpyPacketBufferSize * sizeof(short));
    return 1;
}

void AudioEncoderAdapter::DiscardAudioPacket() {
    while(pcm_packet_pool_->DetectDiscardAudioPacket()){
        if(!pcm_packet_pool_->DiscardAudioPacket()){
            break;
        }
    }
}

int AudioEncoderAdapter::GetAudioPacket() {
	this->DiscardAudioPacket();
	LiveAudioPacket *audioPacket = NULL;
	if (pcm_packet_pool_->GetAudioPacket(&audioPacket, true) < 0) {
		return -1;
	}
	packet_buffer_cursor_ = 0;
	packet_buffer_presentation_time_mills_ = audioPacket->position;
	/**
	 * 在Android平台 录制是单声道的 经过音效处理之后是双声道 channelRatio 2
	 * 在iOS平台 录制的是双声道的 是已经处理音效过后的 channelRatio 1
	 */
	packet_buffer_size_ = audioPacket->size * channel_ratio_;
	if (NULL == packet_buffer_) {
		packet_buffer_ = new short[packet_buffer_size_];
	}
	memcpy(packet_buffer_, audioPacket->buffer, audioPacket->size * sizeof(short));
	int actualSize = this->ProcessAudio();
	if (actualSize > 0 && actualSize < packet_buffer_size_) {
		packet_buffer_cursor_ = packet_buffer_size_ - actualSize;
		memmove(packet_buffer_ + packet_buffer_cursor_, packet_buffer_, actualSize * sizeof(short));
	}
	if (NULL != audioPacket) {
		delete audioPacket;
		audioPacket = NULL;
	}
	return actualSize > 0 ? 1 : -1;
}
