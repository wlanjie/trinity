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
// Created by wlanjie on 2019/4/17.
//

#include "audio_packet_queue.h"
#include "android_xlog.h"

namespace trinity {

AudioPacketQueue::AudioPacketQueue():
      queue_name_("queue"),
      first_(nullptr),
      last_(nullptr),
      packet_size_(0),
      abort_request_(false) {
    Init();
}

AudioPacketQueue::AudioPacketQueue(const char* queueNameParam):
      first_(nullptr),
      last_(nullptr),
      packet_size_(0),
      abort_request_(false) {
    Init();
    queue_name_ = queueNameParam;
}

void AudioPacketQueue::Init() {
    pthread_mutex_init(&lock_, nullptr);
    pthread_cond_init(&condition_, nullptr);
    packet_size_ = 0;
    first_ = nullptr;
    last_ = nullptr;
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
    AudioPacketList *pkt;
    AudioPacketList *pkt1;
    AudioPacket *audioPacket = nullptr;
    pthread_mutex_lock(&lock_);

    for (pkt = first_; pkt != nullptr; pkt = pkt1) {
        pkt1 = pkt->next;
        audioPacket = pkt->pkt;
        if (nullptr != audioPacket) {
            delete audioPacket;
        }
        delete pkt;
    }
    last_ = nullptr;
    first_ = nullptr;
    packet_size_ = 0;
    pthread_mutex_unlock(&lock_);
}

int AudioPacketQueue::Put(AudioPacket *pkt) {
    if (abort_request_) {
        delete pkt;
        return -1;
    }
    auto *pkt1 = new AudioPacketList();
    pkt1->pkt = pkt;
    pkt1->next = nullptr;

    pthread_mutex_lock(&lock_);
    if (last_ == nullptr) {
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
            ret = -1;
            break;
        }

        pkt1 = first_;
        if (pkt1) {
            first_ = pkt1->next;
            if (!first_) {
                last_ = nullptr;
            }
            packet_size_--;
            *pkt = pkt1->pkt;
            delete pkt1;
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

}  // namespace trinity
