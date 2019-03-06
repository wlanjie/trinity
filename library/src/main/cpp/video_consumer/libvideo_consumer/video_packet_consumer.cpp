#include "./video_packet_consumer.h"

#define LOG_TAG "VideoPacketConsumerThread"

// TODO 合并
VideoPacketConsumerThread::VideoPacketConsumerThread() {
    obj = NULL;
}

VideoPacketConsumerThread::~VideoPacketConsumerThread() {
}

int VideoPacketConsumerThread::init(char *videoPath,
                                    int videoWidth, int videoHeight, int videoFrameRate,
                                    int videoBitRate,
                                    int audioSampleRate, int audioChannels, int audioBitRate,
                                    char *audio_codec_name,
                                    int qualityStrategy,
                                    JavaVM *g_jvm, jobject obj) {
    this->g_jvm = g_jvm;
    this->obj = obj;
    int ret = VideoConsumerThread::init(videoPath,
                                        videoWidth, videoHeight, videoFrameRate, videoBitRate,
                                        audioSampleRate, audioChannels, audioBitRate,
                                        audio_codec_name, qualityStrategy);
    packetPool = LiveCommonPacketPool::GetInstance();
    if (ret < 0) {
        LOGI("VideoConsumerThread::Init failed...");
        return ret;
    }

    LOGI("leave VideoPacketConsumerThread::Init...");
    return 1;
}

void VideoPacketConsumerThread::stop() {
    VideoConsumerThread::stop();
}