#ifndef LIVE_BASE_VIDEO_CONSUMER_THREAD_H
#define LIVE_BASE_VIDEO_CONSUMER_THREAD_H

#include "../platform_dependent/platform_4_live_common.h"
#include "./../common/live_thread.h"
#include "livecore/common/packet_pool.h"
#include "livecore/common/audio_packet_pool.h"
#include "../mp4/recording_h264_publisher.h"
#include "../mp4/recording_publisher.h"

#define LIVE_AUDIO_PCM_OUTPUT_CHANNEL 2

#define CLIENT_CANCEL_CONNECT_ERR_CODE               -100199

class VideoConsumerThread : public LiveThread {
public:
    VideoConsumerThread();

    virtual ~VideoConsumerThread();

    int Init(char *videoOutputURI,
             int videoWidth, int videoheight, int videoFrameRate, int videoBitRate,
             int audioSampleRate, int audioChannels, int audioBitRate, char *audio_codec_name,
             int qualityStrategy);

    virtual void Stop();

    void registerPublishTimeoutCallback(int (*on_publish_timeout_callback)(void *context),
                                        void *context);

    void registerHotAdaptiveBitrateCallback(
            int (*hot_adaptive_bitrate_callback)(int maxBitrate, int avgBitrate, int fps,
                                                 void *context), void *context);

    void registerStatisticsBitrateCallback(
            int (*statistics_bitrate_callback)(int sendBitrate, int compressedBitrate,
                                               void *context), void *context);

    int GetH264Packet(LiveVideoPacket **packet);

    int GetAudioPacket(LiveAudioPacket **audioPacket);

protected:
    LivePacketPool *packet_pool_;
    LiveAudioPacketPool *aac_packet_pool_;
    bool stopping_;
    bool connecting_;
    pthread_mutex_t connecting_lock_;
    pthread_mutex_t interrupt_lock_;
    pthread_cond_t interrupt_condition_;

    RecordingPublisher *video_publisher_;
    virtual void Init();
    virtual void BuildPublisherInstance();
    void ReleasePublisher();
    void HandleRun(void *ptr);
};

#endif //LIVE_BASE_VIDEO_CONSUMER_THREAD_H
