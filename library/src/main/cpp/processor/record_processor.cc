#include "record_processor.h"

#define LOG_TAG "SongStudio RecordProcessor"

RecordProcessor::RecordProcessor() {
	audio_samples_ = NULL;
	audio_encoder_ = NULL;
	recording_flag_ = false;
	data_accumulate_time_mills_ = 0;
}

RecordProcessor::~RecordProcessor() {
}

void RecordProcessor::InitAudioBufferSize(int sampleRate, int audioBufferSizeParam) {
	audio_samples_cursor_ = 0;
	audio_buffer_size_ = audioBufferSizeParam;
	audio_sample_rate_ = sampleRate;
	audio_samples_ = new short[audio_buffer_size_];
	packet_pool_ = LiveCommonPacketPool::GetInstance();
	corrector_ = new RecordCorrector();
	audio_buffer_time_mills_ = (float)audio_buffer_size_ * 1000.0f / (float)audio_sample_rate_;
}

void RecordProcessor::FlushAudioBufferToQueue() {
	if (audio_samples_cursor_ > 0) {
		if(NULL == audio_encoder_){
			audio_encoder_ = new AudioProcessEncoderAdapter();
			int audioChannels = 2;
			int audioBitRate = 64 * 1024;
			const char* audioCodecName = "libfdk_aac";
            audio_encoder_->Init(packet_pool_, audio_sample_rate_, audioChannels, audioBitRate, audioCodecName);
		}
		short* packetBuffer = new short[audio_samples_cursor_];
		if (NULL == packetBuffer) {
			return;
		}
		memcpy(packetBuffer, audio_samples_, audio_samples_cursor_ * sizeof(short));
		LiveAudioPacket * audioPacket = new LiveAudioPacket();
		audioPacket->buffer = packetBuffer;
		audioPacket->size = audio_samples_cursor_;
        packet_pool_->PushAudioPacketToQueue(audioPacket);
		audio_samples_cursor_ = 0;
		data_accumulate_time_mills_+=audio_buffer_time_mills_;
//		LOGI("audio_samples_time_mills_ is %.6f SampleCnt Cal timeMills %llu", audio_samples_time_mills_, data_accumulate_time_mills_);
		int correctDurationInTimeMills = 0;
		if(corrector_->DetectNeedCorrect(data_accumulate_time_mills_, audio_samples_time_mills_, &correctDurationInTimeMills)){
			//检测到有问题了, 需要进行修复
			this->CorrectRecordBuffer(correctDurationInTimeMills);
		}
	}
}

int RecordProcessor::CorrectRecordBuffer(int correctTimeMills) {
	LOGI("RecordProcessor::CorrectRecordBuffer... correctTimeMills is %d", correctTimeMills);
	int correctBufferSize = ((float)correctTimeMills / 1000.0f) * audio_sample_rate_;
	LiveAudioPacket * audioPacket = GetSilentDataPacket(correctBufferSize);
    packet_pool_->PushAudioPacketToQueue(audioPacket);
	LiveAudioPacket * accompanyPacket = GetSilentDataPacket(correctBufferSize);
	packet_pool_->pushAccompanyPacketToQueue(accompanyPacket);
	data_accumulate_time_mills_+=correctTimeMills;
	return 0;
}

LiveAudioPacket* RecordProcessor::GetSilentDataPacket(int audioBufferSize) {
	LiveAudioPacket * audioPacket = new LiveAudioPacket();
	audioPacket->buffer = new short[audioBufferSize];
	memset(audioPacket->buffer, 0, audioBufferSize * sizeof(short));
	audioPacket->size = audioBufferSize;
	return audioPacket;
}

int RecordProcessor::PushAudioBufferToQueue(short *samples, int size) {
	if (size <= 0) {
		return size;
	}
	if(!recording_flag_){
		recording_flag_ = true;
		start_time_mills_ = currentTimeMills();
	}
	int samplesCursor = 0;
	int samplesCnt = size;
	while (samplesCnt > 0) {
		if ((audio_samples_cursor_ + samplesCnt) < audio_buffer_size_) {
			this->CpyToAudioSamples(samples + samplesCursor, samplesCnt);
			audio_samples_cursor_ += samplesCnt;
			samplesCursor += samplesCnt;
			samplesCnt = 0;
		} else {
			int subFullSize = audio_buffer_size_ - audio_samples_cursor_;
			this->CpyToAudioSamples(samples + samplesCursor, subFullSize);
			audio_samples_cursor_ += subFullSize;
			samplesCursor += subFullSize;
			samplesCnt -= subFullSize;
			FlushAudioBufferToQueue();
		}
	}
	return size;
}

void RecordProcessor::CpyToAudioSamples(short *sourceBuffer, int cpyLength) {
	if(0 == audio_samples_cursor_){
		audio_samples_time_mills_ = currentTimeMills() - start_time_mills_;
	}
	memcpy(audio_samples_ + audio_samples_cursor_, sourceBuffer, cpyLength * sizeof(short));
}

void RecordProcessor::Destroy() {
	if(NULL != audio_samples_){
		delete[] audio_samples_;
		audio_samples_ = NULL;
	}
	if(NULL != audio_encoder_){
		audio_encoder_->Destroy();
		delete audio_encoder_;
		audio_encoder_ = NULL;
	}
	if(NULL != corrector_){
		delete corrector_;
		corrector_ = NULL;
	}
}
