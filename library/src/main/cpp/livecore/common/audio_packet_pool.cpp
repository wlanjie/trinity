#include "audio_packet_pool.h"

#define LOG_TAG "LiveAudioPacketPool"

LiveAudioPacketPool::LiveAudioPacketPool() {
    audioPacketQueue = NULL;
}

LiveAudioPacketPool::~LiveAudioPacketPool() {
}
//初始化静态成员
LiveAudioPacketPool* LiveAudioPacketPool::instance = new LiveAudioPacketPool();
LiveAudioPacketPool* LiveAudioPacketPool::GetInstance() {
    return instance;
}

/**************************start audio packet queue process**********************************************/
void LiveAudioPacketPool::initAudioPacketQueue() {
    const char* name = "audioPacket AAC Data queue_";
    audioPacketQueue = new LiveAudioPacketQueue(name);
}

void LiveAudioPacketPool::abortAudioPacketQueue() {
    if(NULL != audioPacketQueue){
        audioPacketQueue->abort();
    }
}
void LiveAudioPacketPool::destoryAudioPacketQueue() {
    if(NULL != audioPacketQueue){
        delete audioPacketQueue;
        audioPacketQueue = NULL;
    }
}

int LiveAudioPacketPool::getAudioPacket(LiveAudioPacket **audioPacket, bool block) {
    int result = -1;
    if(NULL != audioPacketQueue){
        result = audioPacketQueue->get(audioPacket, block);
    }
    return result;
}

int LiveAudioPacketPool::getAudioPacketQueueSize() {
    return audioPacketQueue->size();
}

void LiveAudioPacketPool::pushAudioPacketToQueue(LiveAudioPacket* audioPacket) {
    if(NULL != audioPacketQueue){
		audioPacketQueue->put(audioPacket);
    }
}
/**************************end audio packet queue process**********************************************/
