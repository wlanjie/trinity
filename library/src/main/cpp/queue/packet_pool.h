//
// Created by wlanjie on 2019/4/17.
//

#ifndef TRINITY_PACKET_POOL_H
#define TRINITY_PACKET_POOL_H

#include "audio_packet_queue.h"
#include "video_packet_queue.h"

#define AUDIO_PACKET_QUEUE_THRRESHOLD                                        15
#define VIDEO_PACKET_QUEUE_THRRESHOLD                                        60

#define INPUT_CHANNEL_4_ANDROID                                              1

#define AUDIO_PACKET_DURATION_IN_SECS                                        0.04f

namespace trinity {

class PacketPool {
protected:
    PacketPool();
    static PacketPool* instance_;
    AudioPacketQueue* audio_packet_queue_;
    int audio_sample_rate_;
    int channels_;
    VideoPacketQueue* video_packet_queue_;
    AudioPacketQueue* decoder_packet_queue_;
    AudioPacketQueue* accompany_packet_queue_;
private:
    /** 为了丢帧策略所做的实例变量 **/
    int total_discard_video_packet_duration_;
    pthread_rwlock_t rw_lock_;
    int buffer_size_;
    short *buffer_;
    int buffer_cursor_;
    bool DetectDiscardVideoPacket();
    /** 为了计算每一帧的时间长度 **/
    VideoPacket *temp_video_packet_;
    int temp_video_packet_ref_count_;
    int accompany_buffer_size_;
    short* accompany_buffer_;
    int accompany_buffer_cursor_;
    int total_discard_video_packet_duration_copy_;
    pthread_rwlock_t accompany_drop_frame_lock_;

private:
    virtual void RecordDropVideoFrame(int discardVideoPacketSize);

public:
    static PacketPool* GetInstance();
    virtual ~PacketPool();

    virtual void InitAudioPacketQueue(int audio_sample_rate);
    virtual void AbortAudioPacketQueue();
    virtual void DestroyAudioPacketQueue();
    virtual int GetAudioPacket(AudioPacket** audio_packet, bool block);
    virtual void PushAudioPacketToQueue(AudioPacket* audio_packet);
    virtual int GetAudioPacketQueueSize();
    bool DiscardAudioPacket();
    bool DetectDiscardAudioPacket();
    /** 解码出来的伴奏的queue的所有操作 **/
    virtual void InitDecoderAccompanyPacketQueue();
    virtual void AbortDecoderAccompanyPacketQueue();
    virtual void DestoryDecoderAccompanyPacketQueue();
    virtual int GetDecoderAccompanyPacket(AudioPacket **audioPacket, bool block);
    virtual void PushDecoderAccompanyPacketToQueue(AudioPacket *audioPacket);
    virtual void ClearDecoderAccompanyPacketToQueue();
    virtual int GeDecoderAccompanyPacketQueueSize();

    virtual void InitAccompanyPacketQueue(int sampleRate, int channels);
    virtual void AbortAccompanyPacketQueue();
    virtual void DestoryAccompanyPacketQueue();
    virtual int GetAccompanyPacket(AudioPacket **accompanyPacket, bool block);
    virtual void PushAccompanyPacketToQueue(AudioPacket *accompanyPacket);
    virtual int GetAccompanyPacketQueueSize();
    bool DiscardAccompanyPacket();
    bool DetectDiscardAccompanyPacket();

    void InitRecordingVideoPacketQueue();
    void AbortRecordingVideoPacketQueue();
    void DestroyRecordingVideoPacketQueue();
    int GetRecordingVideoPacket(VideoPacket **videoPacket, bool block);
    bool PushRecordingVideoPacketToQueue(VideoPacket *videoPacket);
    int GetRecordingVideoPacketQueueSize();
    void ClearRecordingVideoPacketToQueue();
};

}

#endif //TRINITY_PACKET_POOL_H
