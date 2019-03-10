#ifndef LIVE_BASE_VIDEO_CONSUMER_THREAD_H
#define LIVE_BASE_VIDEO_CONSUMER_THREAD_H

#include "../platform_dependent/platform_4_live_common.h"
#include "./../common/live_thread.h"
#include "livecore/common/packet_pool.h"
#include "livecore/common/audio_packet_pool.h"
#include "livecore/mp4/mux_h264.h"
#include "livecore/mp4/mp4_mux.h"

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

    int GetH264Packet(LiveVideoPacket **packet);

    int GetAudioPacket(LiveAudioPacket **audioPacket);

protected:
    LivePacketPool *packet_pool_;
    LiveAudioPacketPool *aac_packet_pool_;
    bool stopping_;
    Mp4Mux *video_publisher_;
    virtual void Init();
    virtual void BuildPublisherInstance();
    void ReleasePublisher();
    void HandleRun(void *ptr);
};

#endif //LIVE_BASE_VIDEO_CONSUMER_THREAD_H
