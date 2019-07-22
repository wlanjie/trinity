//
// Created by wlanjie on 2019/4/18.
//

#include "audio_packet_pool.h"

namespace trinity {

AudioPacketPool* AudioPacketPool::instance_ = new AudioPacketPool();

AudioPacketPool::AudioPacketPool() {
    audio_packet_queue_ = nullptr;
}

AudioPacketPool *AudioPacketPool::GetInstance() {
    return instance_;
}

AudioPacketPool::~AudioPacketPool() {

}

void AudioPacketPool::InitAudioPacketQueue() {
    const char* name = "audioPacket AAC Data queue_";
    audio_packet_queue_ = new AudioPacketQueue(name);
}

void AudioPacketPool::AbortAudioPacketQueue() {
    if(nullptr != audio_packet_queue_){
        audio_packet_queue_->Abort();
    }
}

void AudioPacketPool::DestroyAudioPacketQueue() {
    if(nullptr != audio_packet_queue_){
        delete audio_packet_queue_;
        audio_packet_queue_ = nullptr;
    }
}

int AudioPacketPool::GetAudioPacket(AudioPacket **audioPacket, bool block) {
    int result = -1;
    if(nullptr != audio_packet_queue_){
        result = audio_packet_queue_->Get(audioPacket, block);
    }
    return result;
}

void AudioPacketPool::PushAudioPacketToQueue(AudioPacket *audioPacket) {
    if (nullptr != audio_packet_queue_){
        audio_packet_queue_->Put(audioPacket);
    }
}

int AudioPacketPool::GetAudioPacketQueueSize() {
    return audio_packet_queue_->Size();
}
}