#ifndef SONGSTUDIO_LIVE_COMMON_PACKET_POOL_H
#define SONGSTUDIO_LIVE_COMMON_PACKET_POOL_H

#include "livecore/common/audio_packet_queue.h"
#include "livecore/common/packet_pool.h"
#include "common_tools.h"

class LiveCommonPacketPool : public LivePacketPool {
private:
    LiveCommonPacketPool();
    static LiveCommonPacketPool *instance;
    /** 边录边合---伴奏的packet queue **/
    LiveAudioPacketQueue *accompanyPacketQueue;
    /** 解码线程---解码出来的伴奏的queue **/
    LiveAudioPacketQueue *decoderAccompanyPacketQueue;

    /** 为了对于伴奏的丢帧策略的引用 **/
    int totalDiscardVideoPacketDurationCopy;
    pthread_rwlock_t accompanyDropFrameRwlock;

    int accompanyBufferSize;
    short *accompanyBuffer;
    int accompanyBufferCursor;

    virtual void RecordDropVideoFrame(int discardVideoPacketDuration);

public:
    static LiveCommonPacketPool *GetInstance(); //工厂方法(用来获得实例)
    virtual ~LiveCommonPacketPool();

    /** 解码出来的伴奏的queue的所有操作 **/
    virtual void initDecoderAccompanyPacketQueue();

    virtual void abortDecoderAccompanyPacketQueue();

    virtual void destoryDecoderAccompanyPacketQueue();

    virtual int getDecoderAccompanyPacket(LiveAudioPacket **audioPacket, bool block);

    virtual void pushDecoderAccompanyPacketToQueue(LiveAudioPacket *audioPacket);

    virtual void clearDecoderAccompanyPacketToQueue();

    virtual int geDecoderAccompanyPacketQueueSize();

    bool discardAccompanyPacket();

    bool detectDiscardAccompanyPacket();

    /** 伴奏的packet queue的所有操作 **/
    virtual void initAccompanyPacketQueue(int sampleRate, int channels);

    virtual void abortAccompanyPacketQueue();

    virtual void destoryAccompanyPacketQueue();

    virtual int getAccompanyPacket(LiveAudioPacket **accompanyPacket, bool block);

    virtual void pushAccompanyPacketToQueue(LiveAudioPacket *accompanyPacket);

    virtual int getAccompanyPacketQueueSize();
};

#endif //SONGSTUDIO_LIVE_COMMON_PACKET_POOL_H
