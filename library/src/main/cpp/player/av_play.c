//
//  av_play.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "av_play.h"
#include <unistd.h>
#include <sys/prctl.h>
#include "audio_play.h"
#include "queue.h"
#include "jni_reflect.h"
#include "audio_filter.h"

static int stop(AVPlayContext* context);

static void on_error(AVPlayContext* context);

static void send_message(AVPlayContext* context, int message) {
    int sig = message;
    write(context->pipe_fd[1], &sig, sizeof(int));
}

static int message_callback(int fd, int events, void *data) {
    AVPlayContext *context = data;
    int message;
    for (int i = 0; i < events; i++) {
        read(fd, &message, sizeof(int));
        switch (message) {
            case message_stop:
//                stop(context);
                LOGE("message_stop");
                if (context->on_complete) {
                    context->on_complete(context);
                }
                break;
            case message_buffer_empty:
                change_status(context, BUFFER_EMPTY);
                break;
            case message_buffer_full:
                change_status(context, BUFFER_FULL);
                break;
            case message_error:
                on_error(context);
                break;
            default:
                break;
        }
    }
    return 1;
}

static void on_error_cb(AVPlayContext* context, int error_code) {
    context->error_code = error_code;
    context->send_message(context, message_error);
}

static void buffer_empty_cb(void *data) {
    AVPlayContext* context = data;
//    if (context->status != BUFFER_EMPTY && !context->eof) {
//        context->send_message(context, message_buffer_empty);
//    }
}

static void buffer_full_cb(void *data) {
    AVPlayContext* context = data;
    if (context->status == BUFFER_EMPTY) {
        context->send_message(context, message_buffer_full);
    }
}

static void reset(AVPlayContext* context) {
    LOGI("enter %s", __func__);
    if (context == NULL) {
        return;
    }
    context->format_context = NULL;
    context->video_codec_ctx = NULL;
    context->video_codec = NULL;
    context->end_of_stream = false;
    context->eof = false;
    context->av_track_flags = 0;
    context->just_audio = false;
    context->video_index = -1;
    context->audio_index = -1;
    context->width = 0;
    context->height = 0;
    context->audio_frame = NULL;
    context->video_frame = NULL;
    context->seeking = 0;
    context->timeout_start = 0;
    clock_reset(context->audio_clock);
    clock_reset(context->video_clock);
    context->error_code = 0;
    context->abort_request = false;
    context->frame_rotation = ROTATION_0;
    context->precise_seek = false;
    context->seek_to = 0;
    context->current_video_packet_pts = 0;
    packet_pool_reset(context->packet_pool);
    change_status(context, IDEL);
    LOGI("leave %s", __func__);
}

static inline void set_buffer_time(AVPlayContext* context) {
    int64_t duration = context->duration / 1000;
    float buffer_time_length = context->buffer_time_length > duration ? duration : context->buffer_time_length;
    AVRational time_base;
    if (context->av_track_flags & AUDIO_FLAG) {
        time_base = context->format_context->streams[context->audio_index]->time_base;
        LOGE("audio max duration: %f", av_q2d(time_base));
        queue_set_duration(context->audio_packet_queue,
                           (uint64_t) (buffer_time_length / av_q2d(time_base)));
        context->audio_packet_queue->empty_cb = buffer_empty_cb;
        context->audio_packet_queue->full_cb = buffer_full_cb;
        context->audio_packet_queue->cb_data = context;
    }
    if (context->av_track_flags & VIDEO_FLAG) {
        time_base = context->format_context->streams[context->video_index]->time_base;
        LOGE("video max duration: %f", av_q2d(time_base));
        queue_set_duration(context->video_packet_queue,
                           (uint64_t) (buffer_time_length / av_q2d(time_base)));
        context->video_packet_queue->empty_cb = buffer_empty_cb;
        context->video_packet_queue->full_cb = buffer_full_cb;
        context->video_packet_queue->cb_data = context;
    }
}

AVPlayContext* av_play_create(JNIEnv *env, jobject instance, int play_create, int sample_rate) {
    AVPlayContext* context = (AVPlayContext *) malloc(sizeof(AVPlayContext));
//    memset(context, 0, sizeof(AVPlayContext));
    (*env)->GetJavaVM(env, &context->vm);
    context->play_object = (*env)->NewGlobalRef(env, instance);
    jni_reflect_java_class(&context->java_class, env);
    context->run_android_version = play_create;
    context->sample_rate = sample_rate;
    context->buffer_size_max = default_buffer_size;
    context->buffer_time_length = default_buffer_time;
    context->force_sw_decode = false;
    context->read_timeout = default_read_timeout;
    context->audio_packet_queue = queue_create(100);
    context->video_packet_queue = queue_create(100);
    context->video_frame_pool = frame_pool_create(6);
    context->video_frame_queue = frame_queue_create(4);
    context->audio_frame_pool = frame_pool_create(12);
    context->audio_frame_queue = frame_queue_create(10);
    context->packet_pool = packet_pool_create(400);
    context->audio_clock = clock_create();
    context->video_clock = clock_create();
    pthread_mutex_init(&context->media_codec_mutex, NULL);
    pthread_mutex_init(&context->eof_mutex_t, NULL);
    pthread_cond_init(&context->eof_cond_t, NULL);
    av_register_all();
    avfilter_register_all();
    context->audio_filter_context = audio_filter_create();
    context->main_looper = ALooper_forThread();
    pipe(context->pipe_fd);
    if (1 != ALooper_addFd(context->main_looper, context->pipe_fd[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
                      message_callback, context)) {
        LOGE("error. when add fd to main looper");
    }
//    context->change_status = change_status;
    context->send_message = send_message;
    context->on_error = on_error_cb;
    context->change_status = NULL;
    reset(context);
    return context;
}

static int audio_codec_init(AVPlayContext* context) {
    LOGI("enter: %s", __func__);
    context->audio_codec = avcodec_find_decoder(
            context->format_context->streams[context->audio_index]->codecpar->codec_id);
    if (context->audio_codec == NULL) {
        LOGE("could not find audio codec\n");
        return 202;
    }
    context->audio_codec_context = avcodec_alloc_context3(context->audio_codec);
    avcodec_parameters_to_context(context->audio_codec_context,
                                  context->format_context->streams[context->audio_index]->codecpar);
    if (avcodec_open2(context->audio_codec_context, context->audio_codec, NULL) < 0) {
        avcodec_free_context(&context->audio_codec_context);
        LOGE("could not open audio codec\n");
        return 203;
    }
    context->audio_filter_context->channels = context->audio_codec_context->channels;
    context->audio_filter_context->channel_layout = context->audio_codec_context->channel_layout;
    audio_filter_change_speed(context, 1.0);
    LOGI("leave: %s", __func__);
    return 0;
}

static int sw_codec_init(AVPlayContext* context) {
    context->video_codec = avcodec_find_decoder(
            context->format_context->streams[context->video_index]->codecpar->codec_id);
    if (context->video_codec == NULL) {
        LOGE("could not find video codec\n");
        return 102;
    }
    context->video_codec_ctx = avcodec_alloc_context3(context->video_codec);
    avcodec_parameters_to_context(context->video_codec_ctx,
                                  context->format_context->streams[context->video_index]->codecpar);
    AVDictionary *options = NULL;
    av_dict_set(&options, "threads", "auto", 0);
    av_dict_set(&options, "refcounted_frames", "1", 0);
    if (avcodec_open2(context->video_codec_ctx, context->video_codec, &options) < 0) {
        avcodec_free_context(&context->video_codec_ctx);
        LOGE("could not open video codec\n");
        return 103;
    }
    return 0;
}

static int hw_codec_init(AVPlayContext* context) {
    context->media_codec_context = create_mediacodec_context(context);
    return mediacodec_start(context);
}

void* audio_decode_thread(void * data){
    prctl(PR_SET_NAME, __func__);
    AVPlayContext* context = (AVPlayContext *)data;
    AudioFilterContext* audio_filter_context = context->audio_filter_context;
    int ret = -1;
    int filter_ret = 0;
    AVFrame* decode_frame = av_frame_alloc();
    AVFrame* frame = frame_pool_get_frame(context->audio_frame_pool);
    while (!context->abort_request) {
        if (context->status == PAUSED) {
            usleep(NULL_LOOP_SLEEP_US);
        }
        ret = avcodec_receive_frame(context->audio_codec_context, frame);
        if (ret == 0) {
//            pthread_mutex_lock(audio_filter_context->mutex);
            decode_frame->channels = audio_filter_context->channels;
            decode_frame->channel_layout = audio_filter_context->channel_layout;
            int add_ret = av_buffersrc_add_frame(audio_filter_context->buffer_src_context, frame);
            if (add_ret >= 0) {
                while(1) {
                    filter_ret = av_buffersink_get_frame(audio_filter_context->buffer_sink_context, frame);
                    if(filter_ret < 0 || filter_ret == AVERROR(EAGAIN) || filter_ret == AVERROR_EOF){
                        break;
                    }
                    int64_t time_stamp = av_rescale_q(frame->pts,
                                  context->format_context->streams[context->audio_index]->time_base,
                                  AV_TIME_BASE_Q);
//                    LOGE("audio packet pts: %lld", time_stamp);
                    if (time_stamp < context->seek_to) {
                        // 解码的音频还不到指定seek的位置, 丢掉
                        frame_pool_unref_frame(context->audio_frame_pool, frame);
                    } else {
                        frame_queue_put(context->audio_frame_queue, frame);
                    }
                    frame = frame_pool_get_frame(context->audio_frame_pool);
                }
            } else {
                char err[128];
                av_strerror(add_ret, err, 127);
                err[127] = 0;
                LOGE("add to audio filter error ==>\n %s", err);
            }
//            pthread_mutex_unlock(audio_filter_context->mutex);
        } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            AVPacket *packet = packet_queue_get(context->audio_packet_queue);
            // buffer empty ==> wait  10ms
            // eof          ==> break
            if(packet == NULL){
                if (context->eof) {
                    avcodec_send_packet(context->audio_codec_context, NULL);
                    if (ret == AVERROR_EOF) {
//                        LOGE("context->eof");
                        usleep(BUFFER_EMPTY_SLEEP_US);
                        continue;
//                        break;
                    } else {
                        continue;
                    }
                } else {
                    usleep(BUFFER_EMPTY_SLEEP_US);
                    continue;
                }
            }
            // seek
            if (packet == &context->audio_packet_queue->flush_packet) {
                LOGE("audio decode packet flush");
                frame_queue_flush(context->audio_frame_queue, context->audio_frame_pool);
                avcodec_flush_buffers(context->audio_codec_context);
                continue;
            }
            ret = avcodec_send_packet(context->audio_codec_context, packet);
            packet_pool_unref_packet(context->packet_pool, packet);
            if (ret < 0) {
                LOGE("send audio packet error: %d msg: %s", ret, av_err2str(ret));
//                context->on_error(context, ERROR_AUDIO_DECODE_SEND_PACKET);
//                break;
                usleep(BUFFER_EMPTY_SLEEP_US);
                continue;
            }
        } else if (ret == AVERROR(EINVAL)) {
            LOGE("ret == AVERROR(EINVAL)");
            context->on_error(context, ERROR_AUDIO_DECODE_CODEC_NOT_OPENED);
            break;
        } else {
            LOGE("else error");
            context->on_error(context, ERROR_AUDIO_DECODE_RECIVE_FRAME);
            break;
        }
    }
    av_frame_free(&decode_frame);
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}

static inline int drop_video_packet(AVPlayContext* context){
    AVPacket* packet = packet_queue_get(context->video_packet_queue);
    if(packet != NULL){
        if (packet != &context->video_packet_queue->flush_packet) {
            int64_t time_stamp = av_rescale_q(packet->pts,
                                              context->format_context->streams[context->video_index]->time_base,
                                              AV_TIME_BASE_Q);
            packet_pool_unref_packet(context->packet_pool, packet);
            int64_t diff = time_stamp - context->audio_clock->pts;
            if (diff > 0) {
                usleep((useconds_t) diff);
            }
        } else {
            frame_queue_flush(context->video_frame_queue, context->video_frame_pool);
            if (!context->is_sw_decode) {
                mediacodec_flush(context);
            }
        }
    }else{
        if (context->eof) {
            return -1;
        }
        usleep(NULL_LOOP_SLEEP_US);
    }
    return 0;
}

void* video_decode_sw_thread(void* data){
    prctl(PR_SET_NAME, __func__);
    AVPlayContext* context = (AVPlayContext *)data;
    int ret;
    AVFrame* frame = frame_pool_get_frame(context->video_frame_pool);
    LOGE("enter: %s abort_request: %d", __func__, context->abort_request);
    while (!context->abort_request) {
        if (context->just_audio) {
            // 如果只播放音频  按照音视频同步的速度丢包
            if( -1 == drop_video_packet(context)){
                break;
            }
        } else {
            ret = avcodec_receive_frame(context->video_codec_ctx, frame);
            if (ret == 0) {
                frame->FRAME_ROTATION = context->frame_rotation;
                frame_queue_put(context->video_frame_queue, frame);
                usleep(2000);
                frame = frame_pool_get_frame(context->video_frame_pool);
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                AVPacket *packet = packet_queue_get(context->video_packet_queue);
                // buffer empty ==> wait  10ms
                // eof          ==> break
                if (packet == NULL) {
                    if (context->eof) {
                        avcodec_send_packet(context->video_codec_ctx, NULL);
                        if (ret == AVERROR_EOF) {
                            break;
                        } else {
                            continue;
                        }
                    } else {
                        LOGE("video buffer empty!!!!!!!!!");
                        usleep(BUFFER_EMPTY_SLEEP_US);
                        continue;
                    }
                }
                // seek
                if (packet == &context->video_packet_queue->flush_packet) {
                    frame_queue_flush(context->video_frame_queue, context->video_frame_pool);
                    avcodec_flush_buffers(context->video_codec_ctx);
                    continue;
                }
                ret = avcodec_send_packet(context->video_codec_ctx, packet);
                packet_pool_unref_packet(context->packet_pool, packet);
                if (ret < 0) {
                    LOGE("%s send packet error: %d", __func__, ret);
                    context->on_error(context, ERROR_VIDEO_SW_DECODE_SEND_PACKET);
                    break;
                }
            } else if (ret == AVERROR(EINVAL)) {
                LOGE("%s avcodec_receive_frame AVERROR(EINVAL)", __func__);
                context->on_error(context, ERROR_VIDEO_SW_DECODE_CODEC_NOT_OPENED);
                break;
            } else {
                LOGE("%s avcodec_receive_frame ret: %d", __func__, ret);
                context->on_error(context, ERROR_VIDEO_SW_DECODE_RECIVE_FRAME);
                break;
            }
        }
    }
    context->error_code = BUFFER_FLAG_END_OF_STREAM;
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}

void clear_frame_queue(AVPlayContext* context) {
    LOGE("enter: %s", __func__);
    while (1) {
        if (context->video_frame_queue->count == 0) {
            break;
        }
        AVFrame* frame = frame_queue_get(context->video_frame_queue);
        if (!frame) {
            break;
        }
        if (frame == &context->video_frame_queue->flush_frame) {
            break;
        }
        mediacodec_release_buffer(context, frame);
        frame_pool_unref_frame(context->video_frame_pool, frame);
    }
//    frame_queue_flush(context->video_frame_queue, context->video_frame_pool);
    frame_queue_flush(context->audio_frame_queue, context->audio_frame_pool);
    packet_queue_flush(context->audio_packet_queue, context->packet_pool);
    avcodec_flush_buffers(context->audio_codec_context);
}

void* video_decode_hw_thread(void* data){
    LOGE("enter %s", __func__);
    prctl(PR_SET_NAME, __func__);
    AVPlayContext* context = (AVPlayContext *)data;
//    mediacodec_start(context);
    int ret;
    AVPacket* packet = NULL;
    AVFrame* frame = frame_pool_get_frame(context->video_frame_pool);
    int64_t seek_time = 0;
    int64_t current_pts = 0;
    while (!context->abort_request) {
        if (context->just_audio) {
            // 如果只播放音频  按照音视频同步的速度丢包
            if (-1 == drop_video_packet(context)){
                break;
            }
        } else {
            ret = mediacodec_receive_frame(context, frame);
            if (ret == BUFFER_FLAG_END_OF_STREAM) {
                context->error_code = BUFFER_FLAG_END_OF_STREAM;
                LOGE("frame->flags & BUFFER_FLAG_END_OF_STREAM: %d",frame->flags & BUFFER_FLAG_END_OF_STREAM);
                continue;
            }
            if (ret >= 0) {
                current_pts = frame->pts / 1000;
                if (context->precise_seek) {
                    // 精准seek
                    // 往后seek
                    if (current_pts < context->seek_to) {
                        mediacodec_release_buffer(context, frame);
                    } else {
                        // 只显示一帧
                        frame->FRAME_ROTATION = context->frame_rotation;
                        frame_queue_put(context->video_frame_queue, frame);
                        frame = frame_pool_get_frame(context->video_frame_pool);
                        if (context->on_seeking && context->status == SEEKING) {
                            context->on_seeking(context);
                            context->status = SEEK_COMPLETE;
                        }
                    }
                } else {
                    // 按时间显示到固定帧
                    // 往前seek
                    bool seek = context->seek_to > current_pts;
                    if (seek) {
                        // seek时按顺序解码, 但是跳帧显示, 100ms显示一帧
                        if (current_pts < seek_time) {
                            mediacodec_release_buffer(context, frame);
                            continue;
                        } else {
                            seek_time = current_pts + 100;
                        }
                    } else {
                        seek_time = 0;
                    }
//                    LOGE("frame count: %d size: %d width: %d line_size: %d height: %d", context->video_frame_queue->count, context->video_frame_queue->size, frame->width, frame->linesize[0], frame->height);
                    frame->FRAME_ROTATION = context->frame_rotation;
                    frame_queue_put(context->video_frame_queue, frame);
                    frame = frame_pool_get_frame(context->video_frame_pool);
                    if (context->on_seeking && seek) {
                        context->on_seeking(context);
                        context->status = SEEK_COMPLETE;
                    }
                }
            }

            // queue mediacodec input
            if (packet == NULL) {
                packet = packet_queue_get(context->video_packet_queue);
            }
            // buffer empty ==> wait  10ms
            // eof          ==> break
            if (packet == NULL) {
                if (context->eof && !context->end_of_stream) {
                    LOGE("context->eof mediacodec_end_of_stream ret: %d current_pts: %lld packet_pts: %lld duration: %lld", ret, current_pts, context->current_video_packet_pts, context->duration);
                    context->end_of_stream = true;
                } else if (!context->end_of_stream) {
                    usleep(BUFFER_EMPTY_SLEEP_US);
                    continue;
                }
            }
            // seek
            if (packet == &context->video_packet_queue->flush_packet) {
                LOGE("packet == &context->video_packet_queue->flush_packet");
                packet = NULL;
                continue;
            }
            int buffer_index = mediacodec_dequeue_input_buffer_index(context);
//            LOGE("ret: %d buffer_index: %d packet_count: %d", ret, buffer_index, context->video_packet_queue->count);
            if (buffer_index >= 0) {
                if (context->end_of_stream) {
                    mediacodec_end_of_stream(context, buffer_index);
                    LOGE("context->end_of_stream");
                    continue;
                }
                if (!packet) {
                    continue;
                }
                ret = mediacodec_send_packet(context, packet, buffer_index);
                if (0 == ret) {
                    packet_pool_unref_packet(context->packet_pool, packet);
                    packet = NULL;
                } else {
                    // some device AMediacodec input buffer ids count < frame_queue->size
                    // when pause   frame_queue not full
                    // thread will not block in  "frame_queue_put" function
                    if (context->status == PAUSED) {
                        LOGE("video_decode_hw_thread paused");
                        usleep(NULL_LOOP_SLEEP_US);
                    }
                }
            }
        }
    }
    (*context->vm)->DetachCurrentThread(context->vm);
    LOGE("thread ==> %s exit", __func__);
    return NULL;
}

static int av_format_interrupt_cb(void *data) {
    AVPlayContext* context = data;
    if (context->timeout_start == 0) {
        context->timeout_start = clock_get_current_time();
        return 0;
    } else {
        uint64_t time_use = clock_get_current_time() - context->timeout_start;
        if (time_use > context->read_timeout * 1000000) {
            context->on_error(context, -2);
            return 1;
        } else {
            return 0;
        }
    }
}

static inline void flush_packet_queue(AVPlayContext *context) {
    LOGE("enter: %s", __func__);
    if (context->av_track_flags & VIDEO_FLAG) {
        packet_queue_flush(context->video_packet_queue, context->packet_pool);
    }
    if (context->av_track_flags & AUDIO_FLAG) {
        packet_queue_flush(context->audio_packet_queue, context->packet_pool);
    }
}

// 读文件线程
void *read_thread(void *data) {
    prctl(PR_SET_NAME, __func__);
    AVPlayContext *context = (AVPlayContext *) data;
    AVPacket *packet = NULL;
    while (!context->abort_request) {
        if (context->seeking == 1) {
            LOGE("read seeking");
            clear_frame_queue(context);
            mediacodec_seek(context);
            context->seeking = 2;
            int seek_ret = av_seek_frame(context->format_context, -1, (int64_t) (context->seek_to * AV_TIME_BASE / 1000),
                                         AVSEEK_FLAG_BACKWARD);
            if (seek_ret < 0) {
                LOGE("seek faild");
            }
        }
        if (context->audio_packet_queue->total_bytes + context->video_packet_queue->total_bytes >=
            context->buffer_size_max) {
            if (context->status == BUFFER_EMPTY) {
                context->send_message(context, message_buffer_full);
            }
            continue;
        }
        // get a new packet from pool
        if (packet == NULL) {
            packet = packet_pool_get_packet(context->packet_pool);
        }
        // read data to packet
        int ret = av_read_frame(context->format_context, packet);
//        LOGE("context: %p file: %s video_size: %d queue->duration: %lld max_duration: %lld status: %d ret: %d",
//                context->format_context, context->format_context->filename, context->video_packet_queue->count, context->video_packet_queue->duration, context->video_packet_queue->max_duration, context->status,ret);
        if (ret == 0) {
            context->timeout_start = 0;
            if (packet->stream_index == context->video_index) {
                packet_queue_put(context->video_packet_queue, packet);
                context->current_video_packet_pts = av_rescale_q(packet->pts,
                                                  context->format_context->streams[context->video_index]->time_base,
                                                  AV_TIME_BASE_Q) / 1000;
                packet = NULL;
            } else if (packet->stream_index == context->audio_index) {
                int64_t time_stamp = av_rescale_q(packet->pts,
                             context->format_context->streams[context->audio_index]->time_base,
                             AV_TIME_BASE_Q) / 1000;
//                LOGE("audio read time_stamp: %lld seek_to: %lld file: %s", time_stamp, context->seek_to, context->format_context->filename);
                if (time_stamp >= context->seek_to) {
                    packet_queue_put(context->audio_packet_queue, packet);
                }
                packet = NULL;
            } else {
                packet_pool_unref_packet(context->packet_pool, packet);
            }
        } else if (ret == AVERROR_INVALIDDATA) {
            context->timeout_start = 0;
            packet_pool_unref_packet(context->packet_pool, packet);
        } else if (ret == AVERROR_EOF) {
            context->eof = true;
            if (context->status == BUFFER_EMPTY) {
                context->send_message(context, message_buffer_full);
            }
            // 文件读取结束时, 等待seek和stop通知
            pthread_mutex_lock(&context->eof_mutex_t);
            pthread_cond_wait(&context->eof_cond_t, &context->eof_mutex_t);
            pthread_mutex_unlock(&context->eof_mutex_t);
        } else {
            // error
            context->on_error(context, ret);
            LOGE("read file error. error code ==> %d", ret);
            break;
        }
    }
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}

int av_play_play(const char* url, AVPlayContext *context) {
    LOGE("enter %s url: %s time: %f", __func__, url, time);
    reset(context);
    int ret = -1;
    context->format_context = avformat_alloc_context();
    context->format_context->interrupt_callback.callback = av_format_interrupt_cb;
    context->format_context->interrupt_callback.opaque = context;
    if (avformat_open_input(&context->format_context, url, NULL, NULL) != 0) {
        LOGE("can not open url: %s\n", av_err2str(ret));
        ret = 100;
        goto fail;
    }
    if (avformat_find_stream_info(context->format_context, NULL) < 0) {
        LOGE("can not find stream\n");
        ret = 101;
        goto fail;
    }
    int i = av_find_best_stream(context->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (i != AVERROR_STREAM_NOT_FOUND) {
        context->video_index = i;
        context->av_track_flags |= VIDEO_FLAG;
    }
    i = av_find_best_stream(context->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (i != AVERROR_STREAM_NOT_FOUND) {
        context->audio_index = i;
        context->av_track_flags |= AUDIO_FLAG;
    }
    AVCodecParameters *codecpar;

    context->duration = context->format_context->duration / 1000;
    if (context->av_track_flags & AUDIO_FLAG) {
        ret = audio_codec_init(context);
        if (ret != 0) {
            goto fail;
        }
    }
    if (context->av_track_flags & VIDEO_FLAG) {
        int64_t d = av_rescale_q(context->format_context->streams[context->video_index]->duration,
                                 context->format_context->streams[context->video_index]->time_base
                , AV_TIME_BASE_Q);
        codecpar = context->format_context->streams[context->video_index]->codecpar;
        context->width = codecpar->width;
        context->height = codecpar->height;
        if (context->force_sw_decode) {
            context->is_sw_decode = true;
        } else {
            switch (codecpar->codec_id) {
                case AV_CODEC_ID_H264:
                case AV_CODEC_ID_HEVC:
                case AV_CODEC_ID_MPEG4:
                case AV_CODEC_ID_VP8:
                case AV_CODEC_ID_VP9:
                case AV_CODEC_ID_H263:
                    context->is_sw_decode = false;
                    break;
                default:
                    context->is_sw_decode = true;
                    break;
            }
        }
        if (context->is_sw_decode) {
            ret = sw_codec_init(context);
        } else {
            ret = hw_codec_init(context);
        }

        // 如果解码失败, 软硬切换,如果还是失败, 通知上层
        if (ret != 0) {
            context->is_sw_decode = !context->is_sw_decode;
            if (context->is_sw_decode) {
                ret = sw_codec_init(context);
            } else {
                ret = hw_codec_init(context);
            }
        }
        if (ret != 0) {
            goto fail;
        }
        AVStream *v_stream = context->format_context->streams[context->video_index];
        AVDictionaryEntry *m = NULL;
        m = av_dict_get(v_stream->metadata, "rotate", m, AV_DICT_MATCH_CASE);
        if (m != NULL) {
            switch (atoi(m->value)) {
                case 90:
                    context->frame_rotation = ROTATION_90;
                    break;
                case 180:
                    context->frame_rotation = ROTATION_180;
                    break;
                case 270:
                    context->frame_rotation = ROTATION_270;
                    break;
                default:
                    break;
            }
        }
    }
    set_buffer_time(context);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&context->read_stream_thread, &attr, read_thread, context);
    LOGE("context->is_sw_decode: %d rotate: %d width: %d height: %d", context->is_sw_decode, context->frame_rotation, context->width, context->height);
    if (context->av_track_flags & VIDEO_FLAG) {
        if (context->is_sw_decode) {
            pthread_create(&context->video_decode_thread, &attr, video_decode_sw_thread, context);
        } else {
            pthread_create(&context->video_decode_thread, &attr, video_decode_hw_thread, context);
        }
    }
    if (context->av_track_flags & AUDIO_FLAG) {
        pthread_create(&context->audio_decode_thread, &attr, audio_decode_thread, context);
    }
    LOGE("context->change_status(context, PLAYING);");
    change_status(context, PLAYING);
    return ret;

    fail:
    if (context->format_context) {
        avformat_close_input(&context->format_context);
    }

    LOGI("leave %s", __func__);
    return ret;
}

void av_play_set_buffer_time(AVPlayContext* context, float buffer_time) {
    context->buffer_time_length = buffer_time;
    if (context->status != IDEL) {
        set_buffer_time(context);
    }
}

void av_play_set_buffer_size(AVPlayContext* context, int buffer_size) {
    context->buffer_size_max = buffer_size;
}

int av_play_resume(AVPlayContext* context) {
    LOGE("enter: %s", __func__);
    change_status(context, PLAYING);
    return 0;
}

void av_play_pause(AVPlayContext* context) {
    change_status(context, PAUSED);
}

void av_play_seek(AVPlayContext* context, int64_t seek_to) {
    if (seek_to >= context->duration) {
        return;
    }
    int64_t previous_seek_time =  context->seek_to;
    int64_t total_time = context->format_context->duration / 1000;
    LOGE("enter: %s seek_to: %lld pre_seek: %lld duration: %lld frame_size; %d max_size; %d pd: %lld pmd: %lld vc: %d", __func__,
            seek_to, previous_seek_time, context->format_context->duration,
            context->video_frame_queue->count,
            context->video_frame_queue->size,
            context->video_packet_queue->duration,
            context->video_packet_queue->max_duration,
            context->video_packet_queue->count);
    seek_to = seek_to >= 0 ? seek_to : 0;
    seek_to = seek_to <= total_time ? seek_to : total_time;
    context->seek_to = seek_to;

    if (context->end_of_stream) {
        mediacodec_flush(context);
    }
    context->error_code = 0;
    context->eof = false;
    context->end_of_stream = false;
    context->precise_seek = previous_seek_time > seek_to;
    if (previous_seek_time > seek_to) {
        flush_packet_queue(context);
        context->seeking = 1;
    } else {
        // 刷出小于seek值已经解码的音频
        while (context->audio_frame_queue->count > 0) {
            AVFrame* frame = frame_queue_get(context->audio_frame_queue);
            int64_t time_stamp = av_rescale_q(frame->pts,
                                              context->format_context->streams[context->audio_index]->time_base,
                                              AV_TIME_BASE_Q) / 1000;
            frame_pool_unref_frame(context->audio_frame_pool, frame);
            if (time_stamp >= seek_to) {
                break;
            }
        }
//        frame_queue_flush(context->audio_frame_queue, context->audio_frame_pool);
    }

    // 刷出已经读取的音频packet
//    while (context->audio_packet_queue->count > 0) {
//        AVPacket* packet = packet_queue_get(context->audio_packet_queue);
//        if (!packet || packet == &context->audio_packet_queue->flush_packet) {
//            break;
//        }
//        int64_t time_stamp = av_rescale_q(packet->pts,
//                                          context->format_context->streams[context->audio_index]->time_base,
//                                          AV_TIME_BASE_Q) / 1000;
//
//        AVPacket* p1 = packet_queue_peek_last(context->audio_packet_queue);
//        int64_t time_stamp1 = av_rescale_q(p1->pts,
//                                          context->format_context->streams[context->audio_index]->time_base,
//                                          AV_TIME_BASE_Q) / 1000;
//        LOGE("audio seek time_stamp: %lld audio packet size: %d last: %lld", time_stamp, context->audio_packet_queue->count, time_stamp1);
//        if (time_stamp >= seek_to) {
//            break;
//        }
//    }

    AVFrame* frame = frame_queue_peek(context->video_frame_queue);
    if (frame && frame != &context->video_frame_queue->flush_frame) {
        int64_t current_time = 0;
        if (context->is_sw_decode) {
            current_time = av_rescale_q(frame->pts,
                                        context->format_context->streams[context->video_index]->time_base,
                                        AV_TIME_BASE_Q) / 1000;
        } else {
            current_time = frame->pts / 1000;
        }
        if (context->on_seeking && context->seek_to > current_time) {
            context->on_seeking(context);
        }
    }

    change_status(context, SEEKING);

    // seek之后通知av_read_frame线程继续读取packet
    pthread_mutex_lock(&context->eof_mutex_t);
    pthread_cond_signal(&context->eof_cond_t);
    pthread_mutex_unlock(&context->eof_mutex_t);
}

void av_play_set_play_background(AVPlayContext* context, bool play_background) {
    if (context->just_audio && (context->av_track_flags & VIDEO_FLAG)) {
        if (context->is_sw_decode) {
            avcodec_flush_buffers(context->video_codec_ctx);
        } else {
            mediacodec_flush(context);
        }
    }
    context->just_audio = play_background;
}

static inline void clean_queues(AVPlayContext* context) {
    LOGE("enter: %s", __func__);
    AVPacket *packet;
    // clear context->audio_frame audio_frame_queue  audio_packet_queue
    if ((context->av_track_flags & AUDIO_FLAG) > 0) {
        if (context->audio_frame != NULL) {
            if (context->audio_frame != &context->audio_frame_queue->flush_frame) {
                frame_pool_unref_frame(context->audio_frame_pool, context->audio_frame);
            }
        }
        while (1) {
            context->audio_frame = frame_queue_get(context->audio_frame_queue);
            if (context->audio_frame == NULL) {
                break;
            }
            if (context->audio_frame != &context->audio_frame_queue->flush_frame) {
                frame_pool_unref_frame(context->audio_frame_pool, context->audio_frame);
            }
        }
        while (1) {
            packet = packet_queue_get(context->audio_packet_queue);
            if (packet == NULL) {
                break;
            }
            if (packet != &context->audio_packet_queue->flush_packet) {
                packet_pool_unref_packet(context->packet_pool, packet);
            }
        }
    }
    // clear context->video_frame video_frame_queue video_frame_packet
    if ((context->av_track_flags & VIDEO_FLAG) > 0) {
        if (context->video_frame != NULL) {
            if (context->video_frame != &context->video_frame_queue->flush_frame) {
                frame_pool_unref_frame(context->video_frame_pool, context->video_frame);
            }
        }
        while (1) {
            context->video_frame = frame_queue_get(context->video_frame_queue);
            if (context->video_frame == NULL) {
                break;
            }
            if (context->video_frame != &context->video_frame_queue->flush_frame) {
                frame_pool_unref_frame(context->video_frame_pool, context->video_frame);
            }
        }
        while (1) {
            packet = packet_queue_get(context->video_packet_queue);
            if (packet == NULL) {
                break;
            }
            if (packet != &context->video_packet_queue->flush_packet) {
                packet_pool_unref_packet(context->packet_pool, packet);
            }
        }
    }
}

static int stop(AVPlayContext *context) {
    if (context->abort_request) {
        return -1;
    }
    LOGE("enter %s file: %s %p", __func__, context->format_context->filename, context);
    context->abort_request = true;
    // remove buffer call back
    context->audio_packet_queue->empty_cb = NULL;
    context->audio_packet_queue->full_cb = NULL;
    context->video_packet_queue->empty_cb = NULL;
    context->video_packet_queue->full_cb = NULL;

    clean_queues(context);

    // 通知read线程解锁wait操作
    pthread_mutex_lock(&context->eof_mutex_t);
    pthread_cond_signal(&context->eof_cond_t);
    pthread_mutex_unlock(&context->eof_mutex_t);

    // 停止各个thread
    void *thread_res;
    pthread_join(context->read_stream_thread, &thread_res);
    if ((context->av_track_flags & VIDEO_FLAG) > 0) {
        pthread_join(context->video_decode_thread, &thread_res);
        if (context->is_sw_decode) {
            avcodec_close(context->video_codec_ctx);
            avcodec_free_context(&context->video_codec_ctx);
        } else {
            mediacodec_stop(context);
        }
    }

    if ((context->av_track_flags & AUDIO_FLAG) > 0) {
        pthread_join(context->audio_decode_thread, &thread_res);
        avcodec_close(context->audio_codec_context);
        avcodec_free_context(&context->audio_codec_context);
    }

    clean_queues(context);
    avformat_close_input(&context->format_context);
    avformat_free_context(context->format_context);
    context->format_context = NULL;
    reset(context);
    LOGI("leave %s", __func__);
    return 0;
}

int av_play_stop(AVPlayContext* context) {
    if (context == NULL || context->status == IDEL) {
        return 0;
    }
    context->error_code = -1;
    return stop(context);
}

int av_play_release(AVPlayContext* context) {
    if (context->status != IDEL) {
        av_play_stop(context);
    }
    while (context->status != IDEL) {
        usleep(10000);
    }
    ALooper_removeFd(context->main_looper, context->pipe_fd[0]);
    close(context->pipe_fd[1]);
    close(context->pipe_fd[0]);
    frame_queue_free(context->audio_frame_queue);
    frame_queue_free(context->video_frame_queue);
    frame_pool_release(context->audio_frame_pool);
    frame_pool_release(context->video_frame_pool);
    packet_queue_free(context->audio_packet_queue);
    packet_queue_free(context->video_packet_queue);
    packet_pool_release(context->packet_pool);
    clock_free(context->audio_clock);
    clock_free(context->video_clock);
    audio_filter_release(context->audio_filter_context);
    JNIEnv* env = NULL;
    int status = (*context->vm)->GetEnv(context->vm, (void**) &env, JNI_VERSION_1_6);
    if (status < 0) {
        (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    }
    (*env)->DeleteGlobalRef(env, context->play_object);
    jni_free(&context->java_class, env);
    if (status < 0) {
        (*context->vm)->DetachCurrentThread(context->vm);
    }
    pthread_mutex_destroy(&context->media_codec_mutex);
    pthread_mutex_destroy(&context->eof_mutex_t);
    pthread_cond_destroy(&context->eof_cond_t);
    free(context);
    return 0;
}

void change_status(AVPlayContext* context, PlayStatus status) {
    LOGE("change_status: %d", status);
    if (status == BUFFER_FULL) {
        av_play_resume(context);
    } else {
        context->status = status;
    }
    if (context->change_status) {
        context->change_status(context, status);
    }
}

static void on_error(AVPlayContext* context) {
    LOGE("on_error: %d", context->error_code);
}