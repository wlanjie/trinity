#ifndef LIVE_PACKET_POOL_H
#define LIVE_PACKET_POOL_H
#include "audio_packet_queue.h"
#include "video_packet_queue.h"

#define AUDIO_PACKET_QUEUE_THRRESHOLD										15
#define VIDEO_PACKET_QUEUE_THRRESHOLD										60

#define	INPUT_CHANNEL_4_ANDROID												1
#define	INPUT_CHANNEL_4_IOS													2

#define AUDIO_PACKET_DURATION_IN_SECS										0.04f

class LivePacketPool {
protected:
    LivePacketPool(); //注意:构造方法私有
    static LivePacketPool* instance; //惟一实例

    /** 边录边合---人声的packet queue **/
    LiveAudioPacketQueue* audioPacketQueue;
    int audioSampleRate;
    int channels;

    /** 边录边合成---视频的YUV数据帧的queue **/
    LiveVideoPacketQueue* recordingVideoPacketQueue;

private:
    /** 为了丢帧策略所做的实例变量 **/
    int                             totalDiscardVideoPacketDuration;
    pthread_rwlock_t                mRwlock;
    
    int 							bufferSize;
    short* 							buffer;
    int 							bufferCursor;
    
    bool detectDiscardVideoPacket();

    /** 为了计算每一帧的时间长度 **/
    LiveVideoPacket*                tempVideoPacket;
    int                             tempVideoPacketRefCnt;

protected:
    virtual void recordDropVideoFrame(int discardVideoPacketSize);
public:
    static LivePacketPool* GetInstance(); //工厂方法(用来获得实例)
    virtual ~LivePacketPool();

    /** 人声的packet queue的所有操作 **/
    virtual void initAudioPacketQueue(int audioSampleRate);
    virtual void abortAudioPacketQueue();
    virtual void destoryAudioPacketQueue();
    virtual int getAudioPacket(LiveAudioPacket **audioPacket, bool block);
    virtual void pushAudioPacketToQueue(LiveAudioPacket* audioPacket);
    virtual int getAudioPacketQueueSize();
    
    bool discardAudioPacket();
    bool detectDiscardAudioPacket();

    /** 解码出来的伴奏的queue的所有操作 **/
    void initRecordingVideoPacketQueue();
    void abortRecordingVideoPacketQueue();
    void destoryRecordingVideoPacketQueue();
    int getRecordingVideoPacket(LiveVideoPacket **videoPacket, bool block);
    bool pushRecordingVideoPacketToQueue(LiveVideoPacket* videoPacket);
    int getRecordingVideoPacketQueueSize();
    void clearRecordingVideoPacketToQueue();
};

#endif //LIVE_PACKET_POOL_H
