#ifndef LIVE_PACKET_POOL_H
#define LIVE_PACKET_POOL_H

#include "audio_packet_queue.h"
#include "video_packet_queue.h"

#define AUDIO_PACKET_QUEUE_THRRESHOLD                                        15
#define VIDEO_PACKET_QUEUE_THRRESHOLD                                        60

#define    INPUT_CHANNEL_4_ANDROID                                                1
#define    INPUT_CHANNEL_4_IOS                                                    2

#define AUDIO_PACKET_DURATION_IN_SECS                                        0.04f

class LivePacketPool {
protected:
    LivePacketPool();
    static LivePacketPool *instance_;

    /** 边录边合---人声的packet queue **/
    LiveAudioPacketQueue *audio_packet_queue_;
    int audio_sample_rate_;
    int channels_;
    /** 边录边合成---视频的YUV数据帧的queue **/
    LiveVideoPacketQueue *recording_video_packet_queue_;
private:
    /** 为了丢帧策略所做的实例变量 **/
    int total_discard_video_packet_duration_;
    pthread_rwlock_t rw_lock_;

    int buffer_size_;
    short *buffer_;
    int buffer_cursor_;
    bool DetectDiscardVideoPacket();
    /** 为了计算每一帧的时间长度 **/
    LiveVideoPacket *temp_video_packet_;
    int temp_video_packet_ref_count_;

protected:
    virtual void RecordDropVideoFrame(int discardVideoPacketSize);

public:
    static LivePacketPool *GetInstance(); //工厂方法(用来获得实例)
    virtual ~LivePacketPool();

    /** 人声的packet queue的所有操作 **/
    virtual void InitAudioPacketQueue(int audioSampleRate);

    virtual void AbortAudioPacketQueue();

    virtual void DestroyAudioPacketQueue();

    virtual int GetAudioPacket(LiveAudioPacket **audioPacket, bool block);

    virtual void PushAudioPacketToQueue(LiveAudioPacket *audioPacket);

    virtual int GetAudioPacketQueueSize();

    bool DiscardAudioPacket();

    bool DetectDiscardAudioPacket();

    /** 解码出来的伴奏的queue的所有操作 **/
    void InitRecordingVideoPacketQueue();

    void AbortRecordingVideoPacketQueue();

    void DestroyRecordingVideoPacketQueue();

    int GetRecordingVideoPacket(LiveVideoPacket **videoPacket, bool block);

    bool PushRecordingVideoPacketToQueue(LiveVideoPacket *videoPacket);

    int GetRecordingVideoPacketQueueSize();

    void ClearRecordingVideoPacketToQueue();
};

#endif //LIVE_PACKET_POOL_H
