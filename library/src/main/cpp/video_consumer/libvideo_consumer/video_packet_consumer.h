#ifndef SONGSTUDIO_BASE_VIDEO_PACKET_CONSUMER_H
#define SONGSTUDIO_BASE_VIDEO_PACKET_CONSUMER_H

#include "livecore/consumer/video_consumer_thread.h"
#include "live_common_packet_pool.h"
#include <map>

class VideoPacketConsumerThread : public VideoConsumerThread {
public:
    VideoPacketConsumerThread();

    virtual ~VideoPacketConsumerThread();

    virtual int init(char *videoPath,
                     int videoWidth, int videoHeight, int videoFrameRate, int videoBitRate,
                     int audioSampleRate, int audioChannels, int audioBitRate,
                     char *audio_codec_name,
                     int qualityStrategy,
                     JavaVM *g_jvm, jobject obj);

    virtual void stop();

protected:
    JavaVM *g_jvm;
    jobject obj;
};

#endif //SONGSTUDIO_BASE_VIDEO_PACKET_CONSUMER_H
