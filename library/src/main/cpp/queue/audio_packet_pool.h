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

#ifndef TRINITY_AUDIO_PACKET_POOL_H
#define TRINITY_AUDIO_PACKET_POOL_H

#include "audio_packet_queue.h"

namespace trinity {

class AudioPacketPool {
 protected:
    AudioPacketPool();
    static AudioPacketPool* instance_;
    AudioPacketQueue* audio_packet_queue_;

 public:
    static AudioPacketPool* GetInstance();
    virtual ~AudioPacketPool();
    virtual void InitAudioPacketQueue();
    virtual void AbortAudioPacketQueue();
    virtual void DestroyAudioPacketQueue();
    virtual int GetAudioPacket(AudioPacket **audioPacket, bool block, bool wait = true);
    virtual void PushAudioPacketToQueue(AudioPacket *audioPacket);
    virtual int GetAudioPacketQueueSize();
};

}  // namespace trinity

#endif  // TRINITY_AUDIO_PACKET_POOL_H
