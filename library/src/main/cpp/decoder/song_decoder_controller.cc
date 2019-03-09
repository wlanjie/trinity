#include "song_decoder_controller.h"

#define LOG_TAG "LiveSongDecoderController"
LiveSongDecoderController::LiveSongDecoderController() {
	accompany_decoder_ = NULL;
	resampler_ = NULL;
	silent_samples_ = NULL;
}

LiveSongDecoderController::~LiveSongDecoderController() {
}

void* LiveSongDecoderController::StartDecoderThread(void *ptr) {
	LOGI("enter LiveSongDecoderController::StartDecoderThread");
	LiveSongDecoderController* decoderController = (LiveSongDecoderController *) ptr;
	int getLockCode = pthread_mutex_lock(&decoderController->lock_);
	while (decoderController->running_) {
		if (decoderController->suspend_flag_) {
			pthread_mutex_lock(&decoderController->suspend_lock_);
			pthread_cond_signal(&decoderController->suspend_condition_);
			pthread_mutex_unlock(&decoderController->suspend_lock_);

			pthread_cond_wait(&decoderController->condition_, &decoderController->lock_);
		} else {
			decoderController->DecodeSongPacket();
			if (decoderController->packet_pool_->geDecoderAccompanyPacketQueueSize() > QUEUE_SIZE_MAX_THRESHOLD) {
				decoderController->accompany_type_ = ACCOMPANY_TYPE_SONG_SAMPLE;
				pthread_cond_wait(&decoderController->condition_, &decoderController->lock_);
			}
		}
	}
	pthread_mutex_unlock(&decoderController->lock_);
	return 0;
}

void LiveSongDecoderController::Init(float packetBufferTimePercent, int vocalSampleRate) {
	volume_ = 1.0f;
	accompany_max_ = 1.0f;
	accompany_type_ = ACCOMPANY_TYPE_SILENT_SAMPLE;
	this->vocal_sample_rate_ = vocalSampleRate;

	//计算计算出伴奏和原唱的bufferSize
	int accompanyByteCountPerSec = vocalSampleRate * CHANNEL_PER_FRAME * BITS_PER_CHANNEL / BITS_PER_BYTE;
	accompany_packet_buffer_size_ = (int) ((accompanyByteCountPerSec / 2) * packetBufferTimePercent);

	//1s的数据与4个buffer的大小哪一个, 选择大的去分配空间
	buffer_queue_cursor_ = 0;
	buffer_queue_size_ = MAX(accompany_packet_buffer_size_ * 4, vocalSampleRate * CHANNEL_PER_FRAME * 1.0f);
	buffer_queue_ = new short[buffer_queue_size_];

	silent_samples_ = new short[accompany_packet_buffer_size_];
	memset(silent_samples_, 0 , accompany_packet_buffer_size_ * 2);
	packet_pool_ = LiveCommonPacketPool::GetInstance();
	packet_pool_->initDecoderAccompanyPacketQueue();
	packet_pool_->initAccompanyPacketQueue(vocalSampleRate, CHANNEL_PER_FRAME);
	InitDecoderThread();
	LOGI("after Init Decoder thread...");
}

void LiveSongDecoderController::StartAccompany(const char *accompanyPath){
	this->SuspendDecoderThread();
	usleep(200 * 1000);
	if(this->InitAccompanyDecoder(accompanyPath) >= 0){
		this->ResumeDecoderThread();
	}
}

void LiveSongDecoderController::SuspendDecoderThread(){
	pthread_mutex_lock(&suspend_lock_);
	pthread_mutex_lock(&lock_);
	suspend_flag_ = true;
	pthread_cond_signal(&condition_);
	pthread_mutex_unlock(&lock_);
	pthread_cond_wait(&suspend_condition_, &suspend_lock_);
	pthread_mutex_unlock(&suspend_lock_);
	accompany_type_ = ACCOMPANY_TYPE_SILENT_SAMPLE;
}

int LiveSongDecoderController::InitAccompanyDecoder(const char *accompanyPath) {
	packet_pool_->clearDecoderAccompanyPacketToQueue();
	this->DestroyResampler();
	this->DestroyAccompanyDecoder();
	accompany_decoder_ = new AACAccompanyDecoder();
	int actualAccompanyPacketBufferSize = accompany_packet_buffer_size_;
	int ret = accompany_decoder_->Init(accompanyPath);
	if(ret > 0){
		//初始化伴奏的采样率
		accompany_sample_rate_ = accompany_decoder_->GetSampleRate();
		if (vocal_sample_rate_ != accompany_sample_rate_) {
			need_resample_ = true;
			resampler_ = new Resampler();
			float ratio = (float)accompany_sample_rate_ / (float)vocal_sample_rate_;
			actualAccompanyPacketBufferSize = ratio * accompany_packet_buffer_size_;
			ret = resampler_->init(accompany_sample_rate_, vocal_sample_rate_, actualAccompanyPacketBufferSize / 2, 2, 2);
			if (ret < 0) {
				LOGI("resampler_ Init error\n");
			}
		} else {
			need_resample_ = false;
		}
		accompany_decoder_->SetPacketBufferSize(actualAccompanyPacketBufferSize);
	}
	return ret;
}

void LiveSongDecoderController::ResumeDecoderThread(){
	suspend_flag_ = false;
	int getLockCode = pthread_mutex_lock(&lock_);
	pthread_cond_signal (&condition_);
	pthread_mutex_unlock (&lock_);
}

void LiveSongDecoderController::PauseAccompany(){
	this->SuspendDecoderThread();
}

void LiveSongDecoderController::ResumeAccompany(){
	this->ResumeDecoderThread();
}

void LiveSongDecoderController::StopAccompany(){
	this->SuspendDecoderThread();
	packet_pool_->clearDecoderAccompanyPacketToQueue();
	this->DestroyResampler();
	this->DestroyAccompanyDecoder();
}

void LiveSongDecoderController::InitDecoderThread() {
	running_ = true;
	suspend_flag_ = true;
	pthread_mutex_init(&lock_, NULL);
	pthread_cond_init(&condition_, NULL);
	pthread_mutex_init(&suspend_lock_, NULL);
	pthread_cond_init(&suspend_condition_, NULL);
	pthread_create(&song_decoder_thread_, NULL, StartDecoderThread, this);
}

void LiveSongDecoderController::DecodeSongPacket() {
	if(NULL == accompany_decoder_){
		return;
	}
	LiveAudioPacket* accompanyPacket = accompany_decoder_->DecodePacket();
	//是否需要重采样
	if (need_resample_ && NULL != resampler_) {
		short* stereoSamples = accompanyPacket->buffer;
		int stereoSampleSize = accompanyPacket->size;
		if (stereoSampleSize > 0) {
			int monoSampleSize = stereoSampleSize / 2;
			short** samples = new short*[2];
			samples[0] = new short[monoSampleSize];
			samples[1] = new short[monoSampleSize];
			for (int i = 0; i < monoSampleSize; i++) {
				samples[0][i] = stereoSamples[2 * i];
				samples[1][i] = stereoSamples[2 * i + 1];
			}
			float transfer_ratio = (float) accompany_sample_rate_ / (float) vocal_sample_rate_;
			int accompanySampleSize = (int) ((float) (monoSampleSize) / transfer_ratio);
//			LOGI("monoSampleSize is %d accompanySampleSize is %d", monoSampleSize, accompanySampleSize);
			uint8_t out_data[accompanySampleSize * 2 * 2];
			int out_nb_bytes = 0;
			resampler_->process(samples, out_data, monoSampleSize, &out_nb_bytes);
			delete[] samples[0];
			delete[] samples[1];
			delete[] stereoSamples;
			if (out_nb_bytes > 0) {
				//			LOGI("out_nb_bytes is %d accompanySampleSize is %d", out_nb_bytes, accompanySampleSize);
				accompanySampleSize = out_nb_bytes / 2;
				short* accompanySamples = new short[accompanySampleSize];
				convertShortArrayFromByteArray(out_data, out_nb_bytes, accompanySamples, 1.0);
				accompanyPacket->buffer = accompanySamples;
				accompanyPacket->size = accompanySampleSize;
			}
		}
	}
	packet_pool_->pushDecoderAccompanyPacketToQueue(accompanyPacket);
}

void LiveSongDecoderController::DestroyDecoderThread() {
//	LOGI("enter LiveSongDecoderController::destoryProduceThread ....");
	running_ = false;
	suspend_flag_ = false;
	void* status;
	int getLockCode = pthread_mutex_lock(&lock_);
	pthread_cond_signal (&condition_);
	pthread_mutex_unlock (&lock_);
	pthread_join(song_decoder_thread_, &status);
	pthread_mutex_destroy(&lock_);
	pthread_cond_destroy(&condition_);

	pthread_mutex_lock(&suspend_lock_);
	pthread_cond_signal(&suspend_condition_);
	pthread_mutex_unlock(&suspend_lock_);
	pthread_mutex_destroy(&suspend_lock_);
	pthread_cond_destroy(&suspend_condition_);
}

int LiveSongDecoderController::BuildSlientSamples(short *samples) {
	LiveAudioPacket* accompanyPacket = new LiveAudioPacket();
	int samplePacketSize = accompany_packet_buffer_size_;
	accompanyPacket->size = samplePacketSize;
	accompanyPacket->buffer = new short[samplePacketSize];
	accompanyPacket->position = -1;
	memcpy(accompanyPacket->buffer, silent_samples_, samplePacketSize * 2);
	memcpy(samples, accompanyPacket->buffer, samplePacketSize * 2);
	this->PushAccompanyPacketToQueue(accompanyPacket);
	return samplePacketSize;
}

int LiveSongDecoderController::ReadSamplesAndProducePacket(short *samples, int size, int *slientSizeArr,
														   int *extraAccompanyTypeArr) {
	int result = -1;
	if(accompany_type_ == ACCOMPANY_TYPE_SILENT_SAMPLE){
		extraAccompanyTypeArr[0] = ACCOMPANY_TYPE_SILENT_SAMPLE;
		slientSizeArr[0] = 0;
		result = this->BuildSlientSamples(samples);
	} else if (accompany_type_ == ACCOMPANY_TYPE_SONG_SAMPLE) {
		extraAccompanyTypeArr[0] = ACCOMPANY_TYPE_SONG_SAMPLE;
		LiveAudioPacket* accompanyPacket = NULL;
		packet_pool_->getDecoderAccompanyPacket(&accompanyPacket, true);
		if (NULL != accompanyPacket) {
			int samplePacketSize = accompanyPacket->size;
			if (samplePacketSize != -1 && samplePacketSize <= size) {
				//copy the raw data to samples
				adjustSamplesVolume(accompanyPacket->buffer, samplePacketSize, volume_ / accompany_max_);
				memcpy(samples, accompanyPacket->buffer, samplePacketSize * 2);
				//push accompany packet_ to accompany queue_
				this->PushAccompanyPacketToQueue(accompanyPacket);
				result = samplePacketSize;
			} else if(-1 == samplePacketSize){
				//解码到最后了
				accompany_type_ = ACCOMPANY_TYPE_SILENT_SAMPLE;
				extraAccompanyTypeArr[0] = ACCOMPANY_TYPE_SILENT_SAMPLE;
				slientSizeArr[0] = 0;
				result = this->BuildSlientSamples(samples);
			}
		} else {
			result = -2;
		}
		if (extraAccompanyTypeArr[0] != ACCOMPANY_TYPE_SILENT_SAMPLE && packet_pool_->geDecoderAccompanyPacketQueueSize() < QUEUE_SIZE_MIN_THRESHOLD) {
			int getLockCode = pthread_mutex_lock(&lock_);
			if (result != -1) {
				pthread_cond_signal(&condition_);
			}
			pthread_mutex_unlock(&lock_);
		}
	}
	return result;
}

void LiveSongDecoderController::PushAccompanyPacketToQueue(LiveAudioPacket *tempPacket) {
	memcpy(buffer_queue_ + buffer_queue_cursor_, tempPacket->buffer, tempPacket->size * sizeof(short));
	buffer_queue_cursor_+=tempPacket->size;
	float position = tempPacket->position;
	delete tempPacket;
	while(buffer_queue_cursor_ >= accompany_packet_buffer_size_){
		short* buffer = new short[accompany_packet_buffer_size_];
		memcpy(buffer, buffer_queue_, accompany_packet_buffer_size_ * sizeof(short));
		int protectedSampleSize = buffer_queue_cursor_ - accompany_packet_buffer_size_;
		memmove(buffer_queue_, buffer_queue_ + accompany_packet_buffer_size_, protectedSampleSize * sizeof(short));
		buffer_queue_cursor_ -= accompany_packet_buffer_size_;
		LiveAudioPacket* actualPacket = new LiveAudioPacket();
		actualPacket->size = accompany_packet_buffer_size_;
		actualPacket->buffer = buffer;
		actualPacket->position = position;
		if(position != -1){
			actualPacket->frameNum = position * this->vocal_sample_rate_;
		} else{
			actualPacket->frameNum = -1;
		}
		packet_pool_->pushAccompanyPacketToQueue(actualPacket);
	}
}

void LiveSongDecoderController::Destroy() {
	LOGI("enter LiveSongDecoderController::Destroy");
	this->DestroyDecoderThread();
	LOGI("after DestroyDecoderThread");
	packet_pool_->abortDecoderAccompanyPacketQueue();
	packet_pool_->destoryDecoderAccompanyPacketQueue();
	this->DestroyResampler();
	this->DestroyAccompanyDecoder();
	if (NULL != silent_samples_) {
		delete[] silent_samples_;
	}
	LOGI("leave LiveSongDecoderController::Destroy");
}

void LiveSongDecoderController::DestroyAccompanyDecoder() {
	if (NULL != accompany_decoder_) {
		accompany_decoder_->Destroy();
		delete accompany_decoder_;
		accompany_decoder_ = NULL;
	}
}

void LiveSongDecoderController::DestroyResampler() {
	if (NULL != resampler_) {
		resampler_->destroy();
		delete resampler_;
		resampler_ = NULL;
	}
}
