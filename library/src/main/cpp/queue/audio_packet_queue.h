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

#ifndef TRINITY_AUDIO_PACKET_QUEUE_H
#define TRINITY_AUDIO_PACKET_QUEUE_H

#include <stdint.h>
#include <string.h>
#include <pthread.h>

namespace trinity {

typedef struct AudioPacket {
    short * buffer;
    uint8_t* data;
    int size;
    float position;
    long frameNum;

    AudioPacket() {
        buffer = nullptr;
        data = nullptr;
        size = 0;
        position = -1;
        frameNum = 0;
    }
    ~AudioPacket() {
        if (nullptr != buffer) {
            delete[] buffer;
            buffer = nullptr;
        }
        if (nullptr != data) {
            delete[] data;
            data = nullptr;
        }
    }
} AudioPacket;

typedef struct AudioPacketList {
    AudioPacket *pkt;
    struct AudioPacketList *next;
    AudioPacketList() {
        pkt = nullptr;
        next = nullptr;
    }
} AudioPacketList;

inline void buildPacketFromBuffer(AudioPacket * audioPacket, short* samples, int sampleSize) {
    short* packetBuffer = new short[sampleSize];
    if (nullptr != packetBuffer) {
        memcpy(packetBuffer, samples, sampleSize * 2);
        audioPacket->buffer = packetBuffer;
        audioPacket->size = sampleSize;
    } else {
        audioPacket->size = -1;
    }
}

class AudioPacketQueue {
 public:
    AudioPacketQueue();
    explicit AudioPacketQueue(const char* queueNameParam);
    ~AudioPacketQueue();

    void Init();
    void Flush();
    int Put(AudioPacket *audioPacket);

    /* return < 0 if aborted, 0 if no packet_ and > 0 if packet_.  */
    int Get(AudioPacket **audioPacket, bool block);
    int Size();
    void Abort();

 private:
    AudioPacketList* first_;
    AudioPacketList* last_;
    int packet_size_;
    bool abort_request_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    const char* queue_name_;
};

}  // namespace trinity

#endif  // TRINITY_AUDIO_PACKET_QUEUE_H
