#include "./video_consumer_thread.h"
#define LOG_TAG "VideoConsumerThread"

VideoConsumerThread::VideoConsumerThread() {
	isStopping = false;
	videoPublisher = NULL;
	isConnecting = false;

	pthread_mutex_init(&connectingLock, NULL);
	pthread_cond_init(&interruptCondition, NULL);
	pthread_mutex_init(&interruptLock, NULL);
}
VideoConsumerThread::~VideoConsumerThread() {
	pthread_mutex_destroy(&connectingLock);
	pthread_cond_destroy(&interruptCondition);
	pthread_mutex_destroy(&interruptLock);
}

void VideoConsumerThread::registerPublishTimeoutCallback(int (*on_publish_timeout_callback)(void *context), void* context) {
	if (NULL != videoPublisher) {
		videoPublisher->registerPublishTimeoutCallback(on_publish_timeout_callback, context);
	}
}

void VideoConsumerThread::registerHotAdaptiveBitrateCallback(int (*hot_adaptive_bitrate_callback)(int maxBitrate, int avgBitrate, int fps, void *context), void *context) {
    if (NULL != videoPublisher) {
        videoPublisher->registerHotAdaptiveBitrateCallback(hot_adaptive_bitrate_callback, context);
    }
}

void VideoConsumerThread::registerStatisticsBitrateCallback(int (*statistics_bitrate_callback)(int sendBitrate, int compressedBitrate, void *context), void *context) {
    if (NULL != videoPublisher) {
        videoPublisher->registerStatisticsBitrateCallback(statistics_bitrate_callback, context);
    }
}

static int fill_aac_packet_callback(LiveAudioPacket** packet, void *context) {
	VideoConsumerThread* videoPacketConsumer = (VideoConsumerThread*) context;
	return videoPacketConsumer->getAudioPacket(packet);
}
int VideoConsumerThread::getAudioPacket(LiveAudioPacket ** audioPacket) {
	if (aacPacketPool->getAudioPacket(audioPacket, true) < 0) {
		LOGI("aacPacketPool->getAudioPacket return negetive value...");
		return -1;
	}
	return 1;
}

static int fill_h264_packet_callback(LiveVideoPacket ** packet, void *context) {
	VideoConsumerThread* videoPacketConsumer = (VideoConsumerThread*) context;
	return videoPacketConsumer->getH264Packet(packet);
}
int VideoConsumerThread::getH264Packet(LiveVideoPacket ** packet) {
	if (packetPool->getRecordingVideoPacket(packet, true) < 0) {
		LOGI("packetPool->getRecordingVideoPacket return negetive value...");
		return -1;
	}
	return 1;
}

void VideoConsumerThread::init() {
	isStopping = false;
	packetPool = LivePacketPool::GetInstance();
	aacPacketPool = LiveAudioPacketPool::GetInstance();
	videoPublisher = NULL;
}

int VideoConsumerThread::init(char* videoOutputURI, int videoWidth, int videoheight, int videoFrameRate, int videoBitRate, int audioSampleRate, int audioChannels, int audioBitRate,
		char* audio_codec_name, int qualityStrategy) {
	init();
	if (NULL == videoPublisher) {
		pthread_mutex_lock(&connectingLock);
		this->isConnecting = true;
		pthread_mutex_unlock(&connectingLock);
		buildPublisherInstance();
		int ret = videoPublisher->init(videoOutputURI, videoWidth, videoheight, videoFrameRate, videoBitRate, audioSampleRate, audioChannels, audioBitRate, audio_codec_name,
				qualityStrategy);

		pthread_mutex_lock(&connectingLock);
		this->isConnecting = false;
		pthread_mutex_unlock(&connectingLock);
		LOGI("videoPublisher->Init return code %d...", ret);

		if (ret < 0 || videoPublisher->isInterrupted()) {
			LOGI("videoPublisher->Init failed...");
			pthread_mutex_lock(&interruptLock);

			this->releasePublisher();

			pthread_cond_signal(&interruptCondition);
			pthread_mutex_unlock(&interruptLock);
			return ret;
		}
		if (!isStopping) {
			videoPublisher->registerFillAACPacketCallback(fill_aac_packet_callback, this);
			videoPublisher->registerFillVideoPacketCallback(fill_h264_packet_callback, this);
		} else {
			LOGI("Client Cancel ...");
			return CLIENT_CANCEL_CONNECT_ERR_CODE;
		}
	}
	return 0;
}

void VideoConsumerThread::releasePublisher() {
	if (NULL != videoPublisher) {
        videoPublisher->stop();
        delete videoPublisher;
        videoPublisher = NULL;
	}
}

void VideoConsumerThread::buildPublisherInstance() {
	videoPublisher = new RecordingH264Publisher();
}

void VideoConsumerThread::stop() {
	LOGI("enter VideoConsumerThread::stop...");
	pthread_mutex_lock(&connectingLock);
	if (isConnecting) {
		LOGI("before interruptPublisherPipe()");
		videoPublisher->interruptPublisherPipe();
		LOGI("after interruptPublisherPipe()");
		pthread_mutex_unlock(&connectingLock);

		//等待init处理完
		pthread_mutex_lock(&interruptLock);
		pthread_cond_wait(&interruptCondition, &interruptLock);
		pthread_mutex_unlock(&interruptLock);

		LOGI("VideoConsumerThread::stop isConnecting return...");
		return;
	}
	pthread_mutex_unlock(&connectingLock);

	int ret = -1;

	isStopping = true;
	packetPool->abortRecordingVideoPacketQueue();
	aacPacketPool->abortAudioPacketQueue();
    long startEndingThreadTimeMills = platform_4_live::getCurrentTimeMills();
	LOGI("before wait publisher encoder_ ...");

	if (videoPublisher != NULL){
		videoPublisher->interruptPublisherPipe();
	}
	if ((ret = wait()) != 0) {
		LOGI("Couldn't cancel VideoConsumerThread: %d", ret);
		//		return;
	}
	LOGI("after wait publisher encoder_ ... %d", (int)(platform_4_live::getCurrentTimeMills() - startEndingThreadTimeMills));

	this->releasePublisher();

	packetPool->destoryRecordingVideoPacketQueue();
	aacPacketPool->destoryAudioPacketQueue();
	LOGI("leave VideoConsumerThread::stop...");
}

void VideoConsumerThread::handleRun(void* ptr) {
	while (mRunning) {
		LOGE("handleRun");
		int ret = videoPublisher->encode();
		if (ret < 0) {
			LOGI("videoPublisher->encode result is invalid, so we will stop encode...");
			break;
		}
	}
}

