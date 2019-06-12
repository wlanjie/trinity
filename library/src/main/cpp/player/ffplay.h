//
//  ffplay.h
//  ffplay
//
//  Created by wlanjie on 16/6/27.
//  Copyright © 2016年 com.wlanjie.ffplay. All rights reserved.
//

#ifndef ffplay_h
#define ffplay_h

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

#include "pthread.h"

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

#define VIDEO_PICTURE_QUEUE_SIZE 6
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct Decoder {
    AVPacket pkt;
    AVPacket pkt_temp;
    struct PacketQueue *queue;
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

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Clock {
    double speed;
    int paused;
    int *queue_serial;
    double pts;
    double last_update;
    double pts_drift;
    int serial;
} Clock;

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

typedef struct PacketQueue {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int abort_request;
    int serial;
    int nb_packets;
    int size;
    MyAVPacketList *first_pkt, *last_pkt;
} PacketQueue;

typedef struct FrameQueue {
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

typedef struct AudioRenderContext {
    void (*Destroy)(struct AudioRenderContext* context);
    void *context;
} AudioRenderContext;

typedef struct VideoRenderContext {
    void (*render_video_frame)(struct VideoRenderContext* context);
    void* context;
} VideoRenderContext;

typedef struct StateContext {
    int (*complete)(struct StateContext* context);
    void* context;
};

typedef struct VideoState {
    // Audio Render callback
    struct AudioRenderContext* audio_render_context;
    struct VideoRenderContext* video_render_context;
    struct StateContext* state_context;

    char *filename;
    int width;
    int height;
    int loop;
    int fast;
    int64_t start_time;
    int64_t end_time;
    int precision_seek;
    int finish;
    
    Clock sample_clock;
    Clock video_clock;
    Clock extclk;
    
    FrameQueue video_queue;
    FrameQueue subtitle_queue;
    FrameQueue sample_queue;
    
    PacketQueue videoq;
    PacketQueue subtitleq;
    PacketQueue audioq;
    
    pthread_cond_t continue_read_thread;
    unsigned int audio_buf_index;
    unsigned int audio_buf_size;
    unsigned int audio_buf1_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    struct AudioParams audio_src;
    double audio_clock;
    double audio_hw_buf_size;
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    double audio_diff_cum;
    int av_sync_type;
    pthread_t read_tid;
    AVStream *audio_st;
    Decoder audio_decode;

    AVStream *video_st;
    int viddec_width;
    int viddec_height;
    Decoder video_decode;
    int queue_attachments_req;
    int video_stream;
    int audio_stream;
    int subtitle_stream;
    
    int eof;
    int paused;
    int last_paused;
    int read_pause_return;
    
    AVFormatContext *ic;
    AudioParams audio_filter_src;
    AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    
    int abort_request;
    double max_frame_duration;
    
    double frame_last_filter_delay;
    int vfilter_idx;
    double frame_last_returned_time;
    int seek_req;
    int64_t seek_pos;
    int64_t seek_rel;
    int seek_flags;
    int step;
    double frame_timer;
    int force_refresh;
    double last_vis_time;

#ifdef __APPLE__
    RDFTContext *rdft;
    FFTSample *rdft_data;
    int rdft_bits;
    int xpos;
    AVStream *subtitle_st;
#endif
    
    struct SwsContext *sub_convert_ctx;
    struct SwsContext *img_convert_ctx;
    
    AVFilterContext *in_video_filter;
    AVFilterContext *out_video_filter;
    AVFilterContext *in_audio_filter;
    AVFilterContext *out_audio_filter;
    AVFilterGraph *agraph;
    
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;

    pthread_t render_thread;
    pthread_mutex_t render_mutex;
    pthread_cond_t render_cond;
} VideoState;

static double rdftspeed = 0.02;

static int64_t audio_callback_time;

/** public method */
int av_start(VideoState* is, const char* file_name);
void av_seek(VideoState* is, uint64_t time);
void av_destroy(VideoState* is);
/** stream pause or resume */
void stream_toggle_pause(VideoState *is);

Frame* frame_queue_peek_writable(FrameQueue* f);
Frame* frame_queue_peek_readable(FrameQueue* f);
Frame* frame_queue_peek(FrameQueue* f);
Frame* frame_queue_peek_last(FrameQueue* f);
Frame* frame_queue_peek_next(FrameQueue* f);
int frame_queue_nb_remaining(FrameQueue *f);
int64_t frame_queue_last_pos(FrameQueue *f);
void frame_queue_next(FrameQueue *f);
int get_master_sync_type(VideoState *is);
double get_clock(Clock *c) ;
double get_master_clock(VideoState *is);
void set_clock(Clock *c, double pts, int serial);
void sync_clock_to_slave(Clock *c, Clock *slave);

int audio_decoder_frame(VideoState *is);
#endif /* ffplay_h */
