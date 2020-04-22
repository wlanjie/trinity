//
//  play_types.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef play_types_h
#define play_types_h

#include <stdbool.h>
#include <pthread.h>
#include <android/native_window_jni.h>
#include <android/looper.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "macro.h"

#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
#include <media/NdkMediaCodec.h>
#endif

struct AVPlayContext;

typedef struct JavaClass {
    jmethodID player_onPlayStatusChanged;
    jmethodID player_onPlayError;

    jobject media_codec_object;
    jmethodID codec_init;
    jmethodID codec_stop;
    jmethodID codec_flush;
    jmethodID codec_dequeueInputBuffer;
    jmethodID codec_queueInputBuffer;
    jmethodID codec_getInputBuffer;
    jmethodID codec_dequeueOutputBufferIndex;
    jmethodID codec_formatChange;
    jmethodID texture_updateTexImage;
    jmethodID texture_getTransformMatrix;
    jmethodID texture_frameAvailable;
    __attribute__((unused))
    jmethodID codec_getOutputBuffer;
    jmethodID codec_releaseOutPutBuffer;
} JavaClass;

typedef struct Clock {
    int64_t update_time;
    int64_t pts;
} Clock;

typedef enum {
    UNINIT = -1,
    IDEL = 0,
    PLAYING,
    PAUSED,
    BUFFER_EMPTY,
    BUFFER_FULL
} PlayStatus;

typedef struct AudioFilterContext {
    AVFilterContext *buffer_sink_context;
    AVFilterContext *buffer_src_context;
    AVFilterGraph *filter_graph;
    AVFilter *abuffer_src;
    AVFilter *abuffer_sink;
//    AVFilterLink *outlink;
    AVFilterInOut *outputs;
    AVFilterInOut *inputs;
    pthread_mutex_t *mutex;
    int channels;
    uint64_t channel_layout;
} AudioFilterContext;

typedef struct PacketPool {
    int index;
    int size;
    int count;
    AVPacket **packets;
} PacketPool;

typedef struct MediaCodecContext {
#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
    AMediaCodec *codec;
    AMediaFormat *format;
#endif
    size_t nal_size;
    int width, height;
    enum AVPixelFormat pix_format;
    enum AVCodecID codec_id;
} MediaCodecContext;

typedef struct PacketQueue {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    AVPacket **packets;
    int read_index;
    int write_index;
    int count;
    int total_bytes;
    unsigned int size;
    uint64_t duration;
    uint64_t max_duration;
    AVPacket flush_packet;

    void (*full_cb)(void *);

    void (*empty_cb)(void *);

    void *cb_data;
} PacketQueue;

typedef struct FramePool {
    int index;
    int size;
    int count;
    AVFrame *frames;
} FramePool;


typedef struct FrameQueue {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    AVFrame **frames;
    int read_index;
    int write_index;
    int count;
    unsigned int size;
    AVFrame flush_frame;
} FrameQueue;

typedef struct AudioPlayContext {
    pthread_mutex_t *lock;
    unsigned int play_pos;
    int buffer_size;
    uint8_t* buffer;
    int frame_size;

    void (*play)(struct AVPlayContext *pd);

    void (*shutdown)();

    void (*release)(struct AudioPlayContext * ctx);

    void (*player_create)(int rate, int channel, struct AVPlayContext *pd);

    int64_t (*get_delta_time)(struct AudioPlayContext * ctx);
} AudioPlayContext;

typedef struct AVPlayContext {
    JavaVM *vm;
    void* priv_data;
    int run_android_version;
    int sample_rate;
    jobject *play_object;
    JavaClass *java_class;
    ANativeWindow* window;

    //用户设置
    int buffer_size_max;
    float buffer_time_length;
    bool force_sw_decode;
    float read_timeout;

    // 播放器状态
    PlayStatus status;

    // 各个thread
    pthread_t read_stream_thread;
    pthread_t audio_decode_thread;
    pthread_t video_decode_thread;

    // 封装
    AVFormatContext *format_context;
    int video_index, audio_index;
    //是否有 音频0x1 视频0x2 字幕0x4
    uint8_t av_track_flags;
    uint64_t timeout_start;

    // packet容器
    PacketQueue *video_packet_queue;
    PacketQueue* audio_packet_queue;
    PacketPool *packet_pool;
    // frame容器
    FramePool *audio_frame_pool;
    FrameQueue *audio_frame_queue;
    FramePool *video_frame_pool;
    FrameQueue *video_frame_queue;

    // 音频
    AudioFilterContext *audio_filter_context;
    AVCodecContext *audio_codec_context;
    AVCodec *audio_codec;
    AVFrame *audio_frame;

    // 软硬解公用
    AVFrame *video_frame;
    int width;
    int height;
    int64_t duration;
    int frame_rotation;

    //软解视频
    AVCodecContext *video_codec_ctx;
    AVCodec *video_codec;
    // 硬解
    MediaCodecContext *media_codec_context;

    // 音视频同步
    Clock *video_clock;
    Clock *audio_clock;

    // 是否软解
    bool is_sw_decode;

    // SEEK
    float seek_to;
    uint8_t seeking;

    // play background
    bool just_audio;

    // end of file
    bool eof;

    bool end_of_stream;

    bool abort_request;

    int media_codec_texture_id;

    pthread_mutex_t media_codec_mutex;

    // error code
    // -1 stop by user
    // -2 read stream time out
    // 1xx init
    // 2xx format and stream
    // 3xx audio decode
    // 4xx video decode sw
    // 5xx video decode hw
    // 6xx audio play
    // 7xx video play  openGL
    int error_code;

    // message
    ALooper *main_looper;
    int pipe_fd[2];

    void (*send_message)(struct AVPlayContext* pd, int message);

    void (*change_status)(struct AVPlayContext* pd, PlayStatus status);

    void (*on_error)(struct AVPlayContext* pd, int error_code);

    void (*on_complete)(struct AVPlayContext* context);

    void (*play_audio)(struct AVPlayContext* context);
} AVPlayContext;
#endif  // play_types_h
