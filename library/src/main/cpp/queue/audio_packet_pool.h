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
    virtual int GetAudioPacket(AudioPacket **audioPacket, bool block);
    virtual void PushAudioPacketToQueue(AudioPacket *audioPacket);
    virtual int GetAudioPacketQueueSize();
};

}

#endif //TRINITY_AUDIO_PACKET_POOL_H
