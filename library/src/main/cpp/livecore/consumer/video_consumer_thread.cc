#include "./video_consumer_thread.h"
#define LOG_TAG "VideoConsumerThread"

VideoConsumerThread::VideoConsumerThread() {
	stopping_ = false;
	video_publisher_ = NULL;
}
VideoConsumerThread::~VideoConsumerThread() {
}

static int fill_aac_packet_callback(LiveAudioPacket** packet, void *context) {
	VideoConsumerThread* videoPacketConsumer = (VideoConsumerThread*) context;
	return videoPacketConsumer->GetAudioPacket(packet);
}
int VideoConsumerThread::GetAudioPacket(LiveAudioPacket **audioPacket) {
	if (aac_packet_pool_->GetAudioPacket(audioPacket, true) < 0) {
		LOGI("aac_packet_pool_->GetAudioPacket return negetive value...");
		return -1;
	}
	return 1;
}

static int fill_h264_packet_callback(LiveVideoPacket ** packet, void *context) {
	VideoConsumerThread* videoPacketConsumer = (VideoConsumerThread*) context;
	return videoPacketConsumer->GetH264Packet(packet);
}
int VideoConsumerThread::GetH264Packet(LiveVideoPacket **packet) {
	if (packet_pool_->GetRecordingVideoPacket(packet, true) < 0) {
		LOGI("packet_pool_->GetRecordingVideoPacket return negetive value...");
		return -1;
	}
	return 1;
}

void VideoConsumerThread::Init() {
	stopping_ = false;
	packet_pool_ = LivePacketPool::GetInstance();
	aac_packet_pool_ = LiveAudioPacketPool::GetInstance();
	video_publisher_ = NULL;
}

int VideoConsumerThread::Init(char *videoOutputURI, int videoWidth, int videoheight, int videoFrameRate,
							  int videoBitRate, int audioSampleRate, int audioChannels, int audioBitRate,
							  char *audio_codec_name, int qualityStrategy) {
	Init();
	if (NULL == video_publisher_) {
		BuildPublisherInstance();
		int ret = video_publisher_->Init(videoOutputURI, videoWidth, videoheight, videoFrameRate, videoBitRate,
										 audioSampleRate, audioChannels, audioBitRate, audio_codec_name,
										 qualityStrategy);

		if (ret < 0) {
			this->ReleasePublisher();
			return ret;
		}
		if (!stopping_) {
			video_publisher_->registerFillAACPacketCallback(fill_aac_packet_callback, this);
			video_publisher_->registerFillVideoPacketCallback(fill_h264_packet_callback, this);
		} else {
			LOGI("Client Cancel ...");
			return CLIENT_CANCEL_CONNECT_ERR_CODE;
		}
	}
	return 0;
}

void VideoConsumerThread::ReleasePublisher() {
	if (NULL != video_publisher_) {
		video_publisher_->Stop();
        delete video_publisher_;
        video_publisher_ = NULL;
	}
}

void VideoConsumerThread::BuildPublisherInstance() {
	video_publisher_ = new MuxH264();
}

void VideoConsumerThread::Stop() {
	LOGI("enter VideoConsumerThread::Stop...");
	int ret = -1;
	stopping_ = true;
	packet_pool_->AbortRecordingVideoPacketQueue();
    aac_packet_pool_->AbortAudioPacketQueue();
    long startEndingThreadTimeMills = platform_4_live::getCurrentTimeMills();
	LOGI("before Wait publisher encoder_ ...");

	if ((ret = Wait()) != 0) {
		LOGI("Couldn't cancel VideoConsumerThread: %d", ret);
		//		return;
	}
	LOGI("after Wait publisher encoder_ ... %d", (int)(platform_4_live::getCurrentTimeMills() - startEndingThreadTimeMills));

	this->ReleasePublisher();

	packet_pool_->DestroyRecordingVideoPacketQueue();
	aac_packet_pool_->DestroyAudioPacketQueue();
	LOGI("leave VideoConsumerThread::Stop...");
}

void VideoConsumerThread::HandleRun(void *ptr) {
	while (running_) {
		LOGE("HandleRun");
		int ret = video_publisher_->Encode();
		if (ret < 0) {
			LOGI("video_publisher_->Encode result is invalid, so we will Stop Encode...");
			break;
		}
	}
}

