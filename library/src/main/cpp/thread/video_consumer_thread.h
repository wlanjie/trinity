//
// Created by wlanjie on 2019/4/17.
//

#ifndef TRINITY_VIDEO_CONSUMER_THREAD_H
#define TRINITY_VIDEO_CONSUMER_THREAD_H

#include <pthread.h>
#include "packet_pool.h"
#include "mp4_muxer.h"
#include "h264_muxer.h"
#include "audio_packet_pool.h"

namespace trinity {

class VideoConsumerThread {
public:
    VideoConsumerThread();
    ~VideoConsumerThread();

    int Init(const char* path, int video_width, int video_height, int frame_rate, int video_bit_Rate,
            int audio_sample_rate, int audio_channels, int audio_bit_rate, char* audio_codec_name);

    void Start();
    void StartAsync();
    int Wait();
    void WaitOnNotify();
    void Notify();
    void Stop();

    int GetH264Packet(VideoPacket** packet);

    int GetAudioPacket(AudioPacket** packet);

protected:
    virtual void HandleRun(void* context);
    static void* StartThread(void* context);
    void Init();
    void Release();

protected:
    pthread_t thread_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    bool running_;

    PacketPool* video_packet_pool_;
    AudioPacketPool* audio_packet_pool_;
    bool stopping_;
    Mp4Muxer* mp4_muxer_;
};

}

#endif //TRINITY_VIDEO_CONSUMER_THREAD_H
