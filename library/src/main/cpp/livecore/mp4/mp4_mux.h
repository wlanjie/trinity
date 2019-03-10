#ifndef RECORDING_PUBLISHER_H
#define RECORDING_PUBLISHER_H

#include "../platform_dependent/platform_4_live_common.h"
#include "../platform_dependent/platform_4_live_ffmpeg.h"
#include "livecore/common/video_packet_queue.h"
#include "livecore/common/audio_packet_queue.h"
#include "livecore/common/packet_pool.h"
#include <map>

#define COLOR_FORMAT            AV_PIX_FMT_YUV420P
#define AUDIO_QUEUE_ABORT_ERR_CODE               -100200
#define VIDEO_QUEUE_ABORT_ERR_CODE               -100201

class Mp4Mux {
public:
    Mp4Mux();

    virtual ~Mp4Mux();

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

    int Encode();
    virtual int Stop();

    /** 声明填充一帧PCM音频的方法 **/
    typedef int (*fill_aac_packet_callback)(LiveAudioPacket **, void *context);

    /** 声明填充一帧H264 packet的方法 **/
    typedef int (*fill_h264_packet_callback)(LiveVideoPacket **, void *context);

protected:
    /** 1、为AVFormatContext增加指定的编码器 **/
    virtual AVStream *AddStream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id,
                                char *codec_name);

    /** 2、开启视频编码器 **/
    virtual int OpenVideo(AVFormatContext *oc, AVCodec *codec, AVStream *st);

    /** 3、开启音频编码器 **/
    int OpenAudio(AVFormatContext *oc, AVCodec *codec, AVStream *st);

    /** 4、为视频流写入一帧数据 **/
    virtual int WriteVideoFrame(AVFormatContext *oc, AVStream *st) = 0;

    /** 5、为音频流写入一帧数据 **/
    virtual int WriteAudioFrame(AVFormatContext *oc, AVStream *st);

    /** 6、关闭视频流 **/
    virtual void CloseVideo(AVFormatContext *oc, AVStream *st);

    /** 7、关闭音频流 **/
    void CloseAudio(AVFormatContext *oc, AVStream *st);

    /** 8、获取视频流的时间戳(秒为单位的double) **/
    virtual double GetVideoStreamTimeInSecs() = 0;

    /** 9、获取音频流的时间戳(秒为单位的double) **/
    double GetAudioStreamTimeInSecs();

    int BuildVideoStream();

    int BuildAudioStream(char *audio_codec_name);

protected:
    // sps and pps data
    uint8_t *header_data_;
    int header_size_;
    int InterleavedWriteFrame(AVFormatContext *s, AVPacket *pkt);
    AVOutputFormat *output_format_;
    AVFormatContext *format_context_;
    AVStream *video_stream_;
    AVStream *audio_stream_;
    AVBitStreamFilterContext *bit_stream_filter_context_;
    double duration_;
    double last_audio_packet_presentation_time_mills_;
    int video_width_;
    int video_height_;
    float video_frame_rate_;
    int video_bit_rate_;
    int audio_sample_rate_;
    int audio_channels_;
    int audio_bit_rate_;

    fill_aac_packet_callback fill_aac_packet_callback_;
    void *fill_aac_packet_context_;
    fill_h264_packet_callback fill_h264_packet_callback_;
    void *fill_h264_packet_context_;
    bool write_header_success_;
};

#endif
