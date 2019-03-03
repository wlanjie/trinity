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

    int init(char *videoOutputURI,
             int videoWidth, int videoheight, int videoFrameRate, int videoBitRate,
             int audioSampleRate, int audioChannels, int audioBitRate, char *audio_codec_name,
             int qualityStrategy,
             const std::map<std::string, int> &configMap);

    virtual void stop();

    void registerPublishTimeoutCallback(int (*on_publish_timeout_callback)(void *context),
                                        void *context);

    void registerHotAdaptiveBitrateCallback(
            int (*hot_adaptive_bitrate_callback)(int maxBitrate, int avgBitrate, int fps,
                                                 void *context), void *context);

    void registerStatisticsBitrateCallback(
            int (*statistics_bitrate_callback)(int sendBitrate, int compressedBitrate,
                                               void *context), void *context);

    int getH264Packet(LiveVideoPacket **packet);

    int getAudioPacket(LiveAudioPacket **audioPacket);

protected:
    LivePacketPool *packetPool;
    LiveAudioPacketPool *aacPacketPool;
    void *statisticsContext;

    bool isStopping;

    bool isConnecting;
    pthread_mutex_t connectingLock;
    pthread_mutex_t interruptLock;
    pthread_cond_t interruptCondition;

    RecordingPublisher *videoPublisher;

    virtual void init();

    virtual void buildPublisherInstance();

    void releasePublisher();

    void handleRun(void *ptr);
};

#endif //LIVE_BASE_VIDEO_CONSUMER_THREAD_H
