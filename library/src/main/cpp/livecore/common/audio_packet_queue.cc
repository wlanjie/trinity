#include "audio_packet_queue.h"
#define LOG_TAG "SongStudioLiveAudioPacketQueue"


LiveAudioPacketQueue::LiveAudioPacketQueue() {
	init();
}

LiveAudioPacketQueue::LiveAudioPacketQueue(const char* queueNameParam) {
	init();
	queueName = queueNameParam;
//	LOGI("queueName is %s ....", queueName);
}

void LiveAudioPacketQueue::init() {
	int initLockCode = pthread_mutex_init(&mLock, NULL);
//	LOGI("initLockCode is %d", initLockCode);
	int initConditionCode = pthread_cond_init(&mCondition, NULL);
//	LOGI("initConditionCode is %d", initConditionCode);
	mNbPackets = 0;
	mFirst = NULL;
	mLast = NULL;
	mAbortRequest = false;
}

LiveAudioPacketQueue::~LiveAudioPacketQueue() {
	LOGI("%s ~LiveAudioPacketQueue ....", queueName);
	flush();
	pthread_mutex_destroy(&mLock);
	pthread_cond_destroy(&mCondition);
}

int LiveAudioPacketQueue::size() {
//	LOGI("%s size ....", queueName);
	pthread_mutex_lock(&mLock);
	int size = mNbPackets;
	pthread_mutex_unlock(&mLock);
	return size;
}

void LiveAudioPacketQueue::flush() {
	LOGI("%s flush .... and this time the queue_ size is %d", queueName, size());
	LiveAudioPacketList *pkt, *pkt1;

	LiveAudioPacket *audioPacket;
	pthread_mutex_lock(&mLock);

	for (pkt = mFirst; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		audioPacket = pkt->pkt;
		if(NULL != audioPacket){
//			LOGI("pkt->pkt is not null will delete....");
			delete audioPacket;
//			LOGI("delete success....");
		}
//		LOGI("delete pkt ....");
		delete pkt;
		pkt = NULL;
//		LOGI("delete success....");
	}
	mLast = NULL;
	mFirst = NULL;
	mNbPackets = 0;

	pthread_mutex_unlock(&mLock);
}

int LiveAudioPacketQueue::put(LiveAudioPacket* pkt) {
	if (mAbortRequest) {
		delete pkt;
		return -1;
	}
//	LOGI("%s put data ....", queueName);
	LiveAudioPacketList *pkt1 = new LiveAudioPacketList();
	if (!pkt1)
		return -1;
	pkt1->pkt = pkt;
	pkt1->next = NULL;

//	LOGI("%s get pthread_mutex_lock in put method....", queueName);
	int getLockCode = pthread_mutex_lock(&mLock);
//	LOGI("%s get pthread_mutex_lock result in put method:  %d", queueName, getLockCode);
	if (mLast == NULL) {
		mFirst = pkt1;
	} else {
		mLast->next = pkt1;
	}

	mLast = pkt1;
	mNbPackets++;
//	LOGI("%s queue_'s mNbPackets : %d", queueName, mNbPackets);

	pthread_cond_signal(&mCondition);
//	LOGI("put realease pthread_mutex_unlock ...");
	pthread_mutex_unlock(&mLock);
//	LOGI("put realease pthread_mutex_unlock success");

	return 0;

}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int LiveAudioPacketQueue::get(LiveAudioPacket **pkt, bool block) {
	LiveAudioPacketList *pkt1;
	int ret;

//	LOGI("%s get pthread_mutex_lock in get method....", queueName);
	int getLockCode = pthread_mutex_lock(&mLock);
//	LOGI("%s get pthread_mutex_lock result in get method:  %d", queueName, getLockCode);
	for (;;) {
		if (mAbortRequest) {
//			LOGI("mAbortRequest ....");
			ret = -1;
			break;
		}

		pkt1 = mFirst;
		//		LOGI("queue_'s mNbPackets : %d", mNbPackets);
		if (pkt1) {
//			LOGI("have data ....");
			mFirst = pkt1->next;
			if (!mFirst)
				mLast = NULL;
			mNbPackets--;
//			LOGI("name is %s queue_'s mNbPackets : %d", queueName, mNbPackets);
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
//			LOGI("%s pthread_cond_wait ....", queueName);
			pthread_cond_wait(&mCondition, &mLock);
		}
	}

	//	LOGI("get realease pthread_mutex_unlock ...");
	pthread_mutex_unlock(&mLock);
	//	LOGI("get realease pthread_mutex_unlock success");
	return ret;

}

void LiveAudioPacketQueue::abort() {
//	LOGI("%s abort ....", queueName);
	pthread_mutex_lock(&mLock);
	mAbortRequest = true;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
//	LOGI("queue_ abort success");
}
