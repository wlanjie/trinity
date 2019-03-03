#ifndef LIVE_AUDIO_PACKET_POOL_H
#define LIVE_AUDIO_PACKET_POOL_H
#include "audio_packet_queue.h"

class LiveAudioPacketPool {
protected:
	LiveAudioPacketPool(); //注意:构造方法私有
    static LiveAudioPacketPool* instance; //惟一实例
    /** 边录边合---人声的packet queue **/
    LiveAudioPacketQueue* audioPacketQueue;

public:
    static LiveAudioPacketPool* GetInstance(); //工厂方法(用来获得实例)
    virtual ~LiveAudioPacketPool();

    /** 人声的packet queue的所有操作 **/
    virtual void initAudioPacketQueue();
    virtual void abortAudioPacketQueue();
    virtual void destoryAudioPacketQueue();
    virtual int getAudioPacket(LiveAudioPacket **audioPacket, bool block);
    virtual void pushAudioPacketToQueue(LiveAudioPacket* audioPacket);
    virtual int getAudioPacketQueueSize();

};

#endif //LIVE_AUDIO_PACKET_POOL_H
