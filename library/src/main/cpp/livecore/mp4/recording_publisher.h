#ifndef RECORDING_PUBLISHER_H
#define RECORDING_PUBLISHER_H

#include "../platform_dependent/platform_4_live_common.h"
#include "../platform_dependent/platform_4_live_ffmpeg.h"
#include "livecore/common/video_packet_queue.h"
#include "livecore/common/audio_packet_queue.h"
#include "livecore/common/packet_pool.h"
#include <map>

#define COLOR_FORMAT            AV_PIX_FMT_YUV420P
#ifndef PUBLISH_DATA_TIME_OUT
#define PUBLISH_DATA_TIME_OUT 15 * 1000
#endif

#define AUDIO_QUEUE_ABORT_ERR_CODE               -100200
#define VIDEO_QUEUE_ABORT_ERR_CODE               -100201

#ifndef PUBLISH_INVALID_FLAG
#define PUBLISH_INVALID_FLAG -1
#endif

// TODO Rename
class RecordingPublisher {
public:
    RecordingPublisher();

    virtual ~RecordingPublisher();

    static int interrupt_cb(void *ctx);

    int detectTimeout();

    virtual int Init(char *videoOutputURI,
                     int videoWidth, int videoHeight, float videoFrameRate, int videoBitRate,
                     int audioSampleRate, int audioChannels, int audioBitRate,
                     char *audio_codec_name,
                     int qualityStrategy);

    virtual void registerFillAACPacketCallback(
            int (*fill_aac_packet)(LiveAudioPacket **, void *context), void *context);

    virtual void registerFillVideoPacketCallback(
            int (*fill_packet_frame)(LiveVideoPacket **, void *context), void *context);

    virtual void registerPublishTimeoutCallback(int (*on_publish_timeout_callback)(void *context),
                                                void *context);

    virtual void registerHotAdaptiveBitrateCallback(
            int (*hot_adaptive_bitrate_callback)(int maxBitrate, int avgBitrate, int fps,
                                                 void *context),
            void *context);
    
    virtual void registerStatisticsBitrateCallback(
            int (*statistics_bitrate_callback)(int sendBitrate, int compressedBitrate,
                                               void *context),
            void *context);

    int Encode();
    virtual int Stop();
    void interruptPublisherPipe() {
        this->publishTimeout = PUBLISH_INVALID_FLAG;
    };

    inline bool isInterrupted() {
        return this->publishTimeout == PUBLISH_INVALID_FLAG;
    };

    /** 声明填充一帧PCM音频的方法 **/
    typedef int (*fill_aac_packet_callback)(LiveAudioPacket **, void *context);

    /** 声明填充一帧H264 packet的方法 **/
    typedef int (*fill_h264_packet_callback)(LiveVideoPacket **, void *context);

    /** 当由于网络问题导致timeout的问题回调 **/
    typedef int (*on_publish_timeout_callback)(void *context);

protected:
    /** 1、为AVFormatContext增加指定的编码器 **/
    virtual AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id,
                                 char *codec_name);

    /** 2、开启视频编码器 **/
    virtual int open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);

    /** 3、开启音频编码器 **/
    int open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st);

    /** 4、为视频流写入一帧数据 **/
    virtual int write_video_frame(AVFormatContext *oc, AVStream *st) = 0;

    /** 5、为音频流写入一帧数据 **/
    virtual int write_audio_frame(AVFormatContext *oc, AVStream *st);

    /** 6、关闭视频流 **/
    virtual void close_video(AVFormatContext *oc, AVStream *st);

    /** 7、关闭音频流 **/
    void close_audio(AVFormatContext *oc, AVStream *st);

    /** 8、获取视频流的时间戳(秒为单位的double) **/
    virtual double getVideoStreamTimeInSecs() = 0;

    /** 9、获取音频流的时间戳(秒为单位的double) **/
    double getAudioStreamTimeInSecs();

    int buildVideoStream();

    int buildAudioStream(char *audio_codec_name);

protected:
    // sps and pps data
    uint8_t *headerData;
    int headerSize;
    int publishTimeout;

    //秒为单位
    int startSendTime = 0;

    int interleavedWriteFrame(AVFormatContext *s, AVPacket *pkt);

    /** 输出视频的上下文以及视频音频流 **/
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *video_st;
    AVStream *audio_st;
    AVBitStreamFilterContext *bsfc;
    double duration;

    /** 音频流数据输出 **/
    double lastAudioPacketPresentationTimeMills;

    /** 音频与视频的编码参数 **/
    int videoWidth;
    int videoHeight;
    float videoFrameRate;
    int videoBitRate;
    int audioSampleRate;
    int audioChannels;
    int audioBitRate;

    /** 注册回调函数 **/
    fill_aac_packet_callback fillAACPacketCallback;
    void *fillAACPacketContext;
    fill_h264_packet_callback fillH264PacketCallback;
    void *fillH264PacketContext;

    bool isWriteHeaderSuccess;
};

#endif
