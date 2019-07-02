//
// Created by wlanjie on 2019-06-29.
//
// ffmpeg_decode只做音频, 视频解码使用, 解码出来的数据都会放到对应的对列中.
// 编辑和合成时都会有对应的解码操作, 但是两者又有不同之处.
// 编辑时需要进行音视频同步和渲染操作. 所以这里不涉及到同步操作
// 合成时不需要进行同步,在合成时需要当前帧的信息, 包括pts等.

#ifndef TRINITY_FFMPEG_DECODE_H
#define TRINITY_FFMPEG_DECODE_H

#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/opt.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/avstring.h"
#include "libswresample/swresample.h"
#include "libavutil/pixdesc.h"

#ifdef __APPLE__
#include "SDL.h"
#include "libswscale/swscale.h"
#include "libavcodec/avfft.h"
#include "libavutil/display.h"
#include "libavutil/eval.h"
#include "libavutil/imgutils.h"
#endif

#include <pthread.h>

#define VIDEO_PICTURE_QUEUE_SIZE 6
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int abort_request;
    int serial;
    int nb_packets;
    int size;
    MyAVPacketList *first_pkt, *last_pkt;
} PacketQueue;

typedef struct Decoder {
    AVPacket pkt;
    AVPacket pkt_temp;
    PacketQueue *queue;
    AVCodecContext *codec_context;
    int pkt_serial;
    int finished;
    int packet_pending;
    pthread_cond_t empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    pthread_t decoder_tid;
} Decoder;

typedef struct Frame {
    AVFrame *frame;
    double pts;
    int64_t pos;
    double duration;
    int serial;
    AVRational sar;
    AVSubtitle sub;
    AVSubtitleRect **subrects;
#ifdef __APPLE__
    SDL_Texture *bmp;
#endif
    int allocated;
    int reallocated;
    int width;
    int height;
    int uploaded;
    int format;
} Frame;

typedef struct {
    Frame queue[FRAME_QUEUE_SIZE];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    PacketQueue *packet_queue;
    int max_size;
    int keep_last;
    int rindex;
    int windex;
    int size;
    int rindex_shown;
} FrameQueue;

typedef struct SeekEvent {
    void (*on_seek_event)(struct SeekEvent* event, int seek_flag);
    void* context;
} SeekEvent;

typedef struct AudioEvent {
    void (*on_audio_prepare_event)(struct AudioEvent* event, int size);
    void* context;
} AudioEvent;

typedef struct StateEvent {
    int (*on_complete_event)(struct StateEvent* event);
    void* context;
} StateContext;

typedef struct {
    // 资源文件, mp4, mov等
    char* file_name;
    // 资源的开始时间, 毫秒
    int64_t start_time;
    // 资源的结束时间, 毫秒
    int64_t end_time;
    // 是否循环解码
    int loop;
    // 是否精准seek, 如果不是精准seek, 从相近的关键帧开始
    // 如果精准seek, seek到关键帧之后, 解码到当前时间
    int precision_seek;
    // 是否结束了
    int finish;
    // 视频解码的对列
    FrameQueue video_frame_queue;
    // 字幕的解码对列
    FrameQueue subtitle_frame_queue;
    // 音频的解码对列
    FrameQueue sample_frame_queue;
    // 视频读取对列
    PacketQueue video_packet_queue;
    // 字幕读取对列
    PacketQueue subtitle_packet_queue;
    // 音频读取对列
    PacketQueue audio_packet_queue;
    // 读取线程
    pthread_t read_thread;
    // 音频流
    AVStream* audio_stream;
    // 音频流的位置
    int audio_stream_index;
    // 音频解码信息
    Decoder audio_decode;
    // 视频流
    AVStream* video_stream;
    // 视频流的位置
    int video_stream_index;
    // 视频解码信息
    Decoder video_decode;
    // 字幕流的位置
    int subtitle_stream_index;
    // 是否结束
    int eof;
    // 是否暂停
    int paused;
    int last_paused;
    int read_pause_return;

    int video_filter_idx;
    // TODO 这两个是否要定义在这里
    double frame_last_filter_delay;
    double frame_last_returned_time;

    // 帧的最大时间
    double max_frame_duration;

    AVFormatContext* ic;
    // avfilter 需要的音频输入数据
    AudioParams audio_filter_src;
    //
    AudioParams audio_tgt;
    //
    AudioParams audio_src;
    // 是否结束
    int abort_request;

    int seek_req;
    int64_t seek_pos;
    int64_t seek_rel;
    int seek_flags;
    int queue_attachments_req;

    AVFilterContext *in_video_filter;
    AVFilterContext *out_video_filter;
    AVFilterContext *in_audio_filter;
    AVFilterContext *out_audio_filter;
    AVFilterGraph *agraph;

    struct SeekEvent* seek_event;
    struct AudioEvent* audio_event;
    struct StateEvent* state_event;
    pthread_cond_t continue_read_thread;
} MediaDecode;

/* return the number of undisplayed frames in the queue */
int frame_queue_nb_remaining(FrameQueue* f);

Frame* frame_queue_peek_readable(FrameQueue* f);

Frame* frame_queue_peek_last(FrameQueue* f);

Frame* frame_queue_peek(FrameQueue* f);

void frame_queue_next(FrameQueue* f);

// 开始解码
int av_decode_start(MediaDecode* decode, const char* file_name);

// seek到某个时间点, 以毫秒为准
void av_seek(MediaDecode* decode, int64_t time);

// 释放解码资源
void av_decode_destroy(MediaDecode* decode);

#endif //TRINITY_FFMPEG_DECODE_H
