//
// Created by wlanjie on 2019/4/17.
//

#include "audio_packet_queue.h"
#include "android_xlog.h"

namespace trinity {

AudioPacketQueue::AudioPacketQueue() {
    Init();
}

AudioPacketQueue::AudioPacketQueue(const char* queueNameParam) {
    Init();
    queue_name_ = queueNameParam;
}

void AudioPacketQueue::Init() {
    pthread_mutex_init(&lock_, NULL);
    pthread_cond_init(&condition_, NULL);
    packet_size_ = 0;
    first_ = NULL;
    last_ = NULL;
    abort_request_ = false;
}

AudioPacketQueue::~AudioPacketQueue() {
    LOGI("%s AudioPacketQueue.", queue_name_);
    Flush();
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&condition_);
}

int AudioPacketQueue::Size() {
    pthread_mutex_lock(&lock_);
    int size = packet_size_;
    pthread_mutex_unlock(&lock_);
    return size;
}

void AudioPacketQueue::Flush() {
    LOGI("%s Flush .... and this time the queue_ Size is %d", queue_name_, Size());
    AudioPacketList *pkt, *pkt1;

    AudioPacket *audioPacket;
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

int AudioPacketQueue::Put(AudioPacket *pkt) {
    if (abort_request_) {
        delete pkt;
        return -1;
    }
//	LOGI("%s Put data ....", queue_name_);
    AudioPacketList *pkt1 = new AudioPacketList();
    if (!pkt1)
        return -1;
    pkt1->pkt = pkt;
    pkt1->next = NULL;

    pthread_mutex_lock(&lock_);
    if (last_ == NULL) {
        first_ = pkt1;
    } else {
        last_->next = pkt1;
    }

    last_ = pkt1;
    packet_size_++;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    return 0;

}

/* return < 0 if aborted, 0 if no packet_ and > 0 if packet_.  */
int AudioPacketQueue::Get(AudioPacket **pkt, bool block) {
    AudioPacketList *pkt1;
    int ret;
    pthread_mutex_lock(&lock_);
    for (;;) {
        if (abort_request_) {
//			LOGI("abort_request_ ....");
            ret = -1;
            break;
        }

        pkt1 = first_;
        if (pkt1) {
            first_ = pkt1->next;
            if (!first_)
                last_ = NULL;
            packet_size_--;
            *pkt = pkt1->pkt;
            delete pkt1;
            pkt1 = NULL;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&condition_, &lock_);
        }
    }

    pthread_mutex_unlock(&lock_);
    return ret;
}

void AudioPacketQueue::Abort() {
    pthread_mutex_lock(&lock_);
    abort_request_ = true;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
}

}