#include "audio_packet_pool.h"

#define LOG_TAG "LiveAudioPacketPool"

LiveAudioPacketPool::LiveAudioPacketPool() {
    audio_packet_queue_ = NULL;
}

LiveAudioPacketPool::~LiveAudioPacketPool() {
}
//初始化静态成员
LiveAudioPacketPool* LiveAudioPacketPool::instance_ = new LiveAudioPacketPool();
LiveAudioPacketPool* LiveAudioPacketPool::GetInstance() {
    return instance_;
}

/**************************start audio packet queue process**********************************************/
void LiveAudioPacketPool::InitAudioPacketQueue() {
    const char* name = "audioPacket AAC Data queue_";
    audio_packet_queue_ = new LiveAudioPacketQueue(name);
}

void LiveAudioPacketPool::AbortAudioPacketQueue() {
    if(NULL != audio_packet_queue_){
        audio_packet_queue_->Abort();
    }
}
void LiveAudioPacketPool::DestroyAudioPacketQueue() {
    if(NULL != audio_packet_queue_){
        delete audio_packet_queue_;
        audio_packet_queue_ = NULL;
    }
}

int LiveAudioPacketPool::GetAudioPacket(LiveAudioPacket **audioPacket, bool block) {
    int result = -1;
    if(NULL != audio_packet_queue_){
        result = audio_packet_queue_->Get(audioPacket, block);
    }
    return result;
}

int LiveAudioPacketPool::GetAudioPacketQueueSize() {
    return audio_packet_queue_->Size();
}

void LiveAudioPacketPool::PushAudioPacketToQueue(LiveAudioPacket *audioPacket) {
    if(NULL != audio_packet_queue_){
        audio_packet_queue_->Put(audioPacket);
    }
}
/**************************end audio packet queue process**********************************************/
