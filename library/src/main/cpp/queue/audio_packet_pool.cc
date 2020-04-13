/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//
// Created by wlanjie on 2019/4/18.
//

#include "audio_packet_pool.h"
#include "android_xlog.h"

namespace trinity {

AudioPacketPool* AudioPacketPool::instance_ = new AudioPacketPool();

AudioPacketPool::AudioPacketPool() {
    audio_packet_queue_ = nullptr;
    pthread_mutex_init(&audio_packet_mutex_, nullptr);
    pthread_cond_init(&audio_packet_cond_, nullptr);
}

AudioPacketPool *AudioPacketPool::GetInstance() {
    return instance_;
}

AudioPacketPool::~AudioPacketPool() {
    pthread_mutex_destroy(&audio_packet_mutex_);
    pthread_cond_destroy(&audio_packet_cond_);
}

void AudioPacketPool::InitAudioPacketQueue() {
    const char* name = "audioPacket AAC Data queue_";
    audio_packet_queue_ = new AudioPacketQueue(name);
}

void AudioPacketPool::AbortAudioPacketQueue() {
    if (nullptr != audio_packet_queue_) {
        audio_packet_queue_->Abort();
    }
}

void AudioPacketPool::DestroyAudioPacketQueue() {
    if (nullptr != audio_packet_queue_) {
        delete audio_packet_queue_;
        audio_packet_queue_ = nullptr;
    }
}

int AudioPacketPool::GetAudioPacket(AudioPacket **audioPacket, bool block, bool wait) {
    if (GetAudioPacketQueueSize() == 0 && wait) {
        pthread_mutex_lock(&audio_packet_mutex_);
        pthread_cond_wait(&audio_packet_cond_, &audio_packet_mutex_);
        pthread_mutex_unlock(&audio_packet_mutex_);
    }
    int result = -1;
    if (nullptr != audio_packet_queue_ && audio_packet_queue_->Size() > 0) {
        result = audio_packet_queue_->Get(audioPacket, block);
    }
    return result;
}

void AudioPacketPool::PushAudioPacketToQueue(AudioPacket *audioPacket) {
    if (nullptr != audio_packet_queue_ && audioPacket != nullptr) {
        audio_packet_queue_->Put(audioPacket);
    }
    pthread_mutex_lock(&audio_packet_mutex_);
    pthread_cond_signal(&audio_packet_cond_);
    pthread_mutex_unlock(&audio_packet_mutex_);
}

int AudioPacketPool::GetAudioPacketQueueSize() {
    return audio_packet_queue_->Size();
}

}  // namespace trinity
