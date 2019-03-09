#include "audio_packet_queue.h"
#define LOG_TAG "SongStudioLiveAudioPacketQueue"


LiveAudioPacketQueue::LiveAudioPacketQueue() {
    Init();
}

LiveAudioPacketQueue::LiveAudioPacketQueue(const char* queueNameParam) {
    Init();
	queue_name_ = queueNameParam;
//	LOGI("queue_name_ is %s ....", queue_name_);
}

void LiveAudioPacketQueue::Init() {
	int initLockCode = pthread_mutex_init(&lock_, NULL);
//	LOGI("initLockCode is %d", initLockCode);
	int initConditionCode = pthread_cond_init(&condition_, NULL);
//	LOGI("initConditionCode is %d", initConditionCode);
	packet_size_ = 0;
	first_ = NULL;
	last_ = NULL;
	abort_request_ = false;
}

LiveAudioPacketQueue::~LiveAudioPacketQueue() {
	LOGI("%s ~LiveAudioPacketQueue ....", queue_name_);
	Flush();
	pthread_mutex_destroy(&lock_);
	pthread_cond_destroy(&condition_);
}

int LiveAudioPacketQueue::Size() {
//	LOGI("%s Size ....", queue_name_);
	pthread_mutex_lock(&lock_);
	int size = packet_size_;
	pthread_mutex_unlock(&lock_);
	return size;
}

void LiveAudioPacketQueue::Flush() {
	LOGI("%s Flush .... and this time the queue_ Size is %d", queue_name_, Size());
	LiveAudioPacketList *pkt, *pkt1;

	LiveAudioPacket *audioPacket;
	pthread_mutex_lock(&lock_);

	for (pkt = first_; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		audioPacket = pkt->pkt;
		if(NULL != audioPacket){
//			LOGI("pkt_->pkt_ is not null will delete....");
			delete audioPacket;
//			LOGI("delete success....");
		}
//		LOGI("delete pkt_ ....");
		delete pkt;
		pkt = NULL;
//		LOGI("delete success....");
	}
	last_ = NULL;
	first_ = NULL;
	packet_size_ = 0;

	pthread_mutex_unlock(&lock_);
}

int LiveAudioPacketQueue::Put(LiveAudioPacket *pkt) {
	if (abort_request_) {
		delete pkt;
		return -1;
	}
//	LOGI("%s Put data ....", queue_name_);
	LiveAudioPacketList *pkt1 = new LiveAudioPacketList();
	if (!pkt1)
		return -1;
	pkt1->pkt = pkt;
	pkt1->next = NULL;

//	LOGI("%s Get pthread_mutex_lock in Put method....", queue_name_);
	int getLockCode = pthread_mutex_lock(&lock_);
//	LOGI("%s Get pthread_mutex_lock result in Put method:  %d", queue_name_, getLockCode);
	if (last_ == NULL) {
		first_ = pkt1;
	} else {
		last_->next = pkt1;
	}

	last_ = pkt1;
	packet_size_++;
//	LOGI("%s queue_'s packet_size_ : %d", queue_name_, packet_size_);

	pthread_cond_signal(&condition_);
//	LOGI("Put realease pthread_mutex_unlock ...");
	pthread_mutex_unlock(&lock_);
//	LOGI("Put realease pthread_mutex_unlock success");

	return 0;

}

/* return < 0 if aborted, 0 if no packet_ and > 0 if packet_.  */
int LiveAudioPacketQueue::Get(LiveAudioPacket **pkt, bool block) {
	LiveAudioPacketList *pkt1;
	int ret;

//	LOGI("%s Get pthread_mutex_lock in Get method....", queue_name_);
	int getLockCode = pthread_mutex_lock(&lock_);
//	LOGI("%s Get pthread_mutex_lock result in Get method:  %d", queue_name_, getLockCode);
	for (;;) {
		if (abort_request_) {
//			LOGI("abort_request_ ....");
			ret = -1;
			break;
		}

		pkt1 = first_;
		//		LOGI("queue_'s packet_size_ : %d", packet_size_);
		if (pkt1) {
//			LOGI("have data ....");
			first_ = pkt1->next;
			if (!first_)
				last_ = NULL;
			packet_size_--;
//			LOGI("name is %s queue_'s packet_size_ : %d", queue_name_, packet_size_);
			*pkt = pkt1->pkt;
			delete pkt1;
			pkt1 = NULL;
			ret = 1;
			break;
		} else if (!block) {
//			LOGI("block ....");
			ret = 0;
			break;
		} else {
//			LOGI("%s pthread_cond_wait ....", queue_name_);
			pthread_cond_wait(&condition_, &lock_);
		}
	}

	//	LOGI("Get realease pthread_mutex_unlock ...");
	pthread_mutex_unlock(&lock_);
	//	LOGI("Get realease pthread_mutex_unlock success");
	return ret;

}

void LiveAudioPacketQueue::Abort() {
//	LOGI("%s Abort ....", queue_name_);
	pthread_mutex_lock(&lock_);
	abort_request_ = true;
	pthread_cond_signal(&condition_);
	pthread_mutex_unlock(&lock_);
//	LOGI("queue_ Abort success");
}
