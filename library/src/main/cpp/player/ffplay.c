//
//  ffplay.c
//  ffplay
//
//  Created by wlanjie on 16/6/27.
//  Copyright © 2016年 com.wlanjie.ffplay. All rights reserved.
//

#include "ffplay.h"
#include "android_xlog.h"

#define FF_ALLOC_EVENT   (SDL_USEREVENT)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control */
#define SDL_VOLUME_STEP (SDL_MIX_MAXVOLUME / 50)

/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

#define CURSOR_HIDE_DELAY 1000000

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

#define USE_ONEPASS_SUBTITLE_RENDER 1

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

#define MAXVOLUME 128

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

static char *wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static int lowres;
static AVPacket flush_pkt;
static int64_t duration = AV_NOPTS_VALUE;

int frame_queue_init( FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last) {
    memset(f, 0, sizeof(FrameQueue));
    int result = pthread_mutex_init(&f->mutex, NULL);
    if (result != 0) {
        av_log(NULL, AV_LOG_FATAL, "Frame Init create mutex: %d\n", result);
        return AVERROR(ENOMEM);
    }
    result = pthread_cond_init(&f->cond, NULL);
    if (result != 0) {
        av_log(NULL, AV_LOG_FATAL, "Frame Init create cond: %d\n", result);
        return AVERROR(ENOMEM);
    }
    f->packet_queue = pktq;
    f->max_size = max_size;
    f->keep_last = keep_last;
    for (int i = 0; i < f->max_size; i++) {
        if (!(f->queue[i].frame = av_frame_alloc())) {
            return AVERROR(ENOMEM);
        }
    }
    return 0;
}

Frame *frame_queue_peek_writable(FrameQueue *f) {
    pthread_mutex_lock(&f->mutex);
    while (f->size >= f->max_size && !f->packet_queue->abort_request) {
        pthread_cond_wait(&f->cond, &f->mutex);
    }
    pthread_mutex_unlock(&f->mutex);
    if (f->packet_queue->abort_request) {
        return NULL;
    }
    return &f->queue[f->windex];
}

Frame *frame_queue_peek_readable(FrameQueue *f) {
    pthread_mutex_lock(&f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->packet_queue->abort_request) {
        LOGE("pthread_cond_wait(&f->cond, &f->mutex);");
        pthread_cond_wait(&f->cond, &f->mutex);
    }
    pthread_mutex_unlock(&f->mutex);
    if (f->packet_queue->abort_request) {
        LOGE("f->packet_queue->abort_request");
        return NULL;
    }
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void frame_queue_push(FrameQueue *f) {
    if (++f->windex == f->max_size) {
        f->windex = 0;
    }
    pthread_mutex_lock(&f->mutex);
    f->size++;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

Frame *frame_queue_peek(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

/* return the number of undisplayed frames in the queue */
int frame_queue_nb_remaining(FrameQueue *f) {
    return f->size - f->rindex_shown;
}

/* return last shown position */
int64_t frame_queue_last_pos(FrameQueue *f) {
    Frame *fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->packet_queue->serial)
        return fp->pos;
    else
        return -1;
}

Frame *frame_queue_peek_last(FrameQueue *f) {
    return &f->queue[f->rindex];
}

Frame *frame_queue_peek_next(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

void frame_queue_unref_item(Frame *vp) {
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

void frame_queue_next(FrameQueue *f) {
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size) {
        f->rindex = 0;
    }
    pthread_mutex_lock(&f->mutex);
    f->size--;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

#ifdef __APPLE__
void free_picture(Frame *vp) {
    if (vp->bmp) {
        SDL_DestroyTexture(vp->bmp);
        vp->bmp = NULL;
    }
}
#endif

void frame_queue_signal(FrameQueue *f) {
    pthread_mutex_lock(&f->mutex);
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

void frame_queue_destory(FrameQueue *f) {
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
#ifdef __APPLE__
        free_picture(vp);
#endif
    }
    pthread_mutex_destroy(&f->mutex);
    pthread_cond_destroy(&f->cond);
}

int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    int result = pthread_mutex_init(&q->mutex, NULL);
    if (result != 0) {
        av_log(NULL, AV_LOG_FATAL, "Packet Init mutex: %d\n", result);
        return AVERROR(ENOMEM);
    }
    result = pthread_cond_init(&q->cond, NULL);
    if (result != 0) {
        av_log(NULL, AV_LOG_FATAL, "Packet Init cond: %d\n", result);
        return AVERROR(ENOMEM);
    }
    q->abort_request = 0;
    return 0;
}

int packet_queue_put_private(PacketQueue *q, AVPacket *packet) {
    MyAVPacketList *pkt1;
    if (q->abort_request) {
        return -1;
    }
    pkt1 = av_malloc(sizeof(MyAVPacketList));
    if (!pkt1) {
        return AVERROR(ENOMEM);
    }
    pkt1->pkt = *packet;
    pkt1->next = NULL;
    if (packet == &flush_pkt) {
        q->serial++;
    }
    pkt1->serial = q->serial;
    if (!q->last_pkt) {
        q->first_pkt = pkt1;
    } else {
        q->last_pkt->next = pkt1;
    }
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    pthread_cond_signal(&q->cond);
    return 0;
}

void packet_queue_start(PacketQueue *q) {
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, &flush_pkt);
    pthread_mutex_unlock(&q->mutex);
}

void packet_queue_abort(PacketQueue *q) {
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 1;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

void packet_queue_flush(PacketQueue *q) {
    MyAVPacketList *pkt, *pkt1;
    pthread_mutex_lock(&q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    pthread_mutex_unlock(&q->mutex);
}

void packet_queue_destroy(PacketQueue *q) {
    packet_queue_flush(q);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void set_clock_at(Clock *c, double pts, int serial, double time) {
    c->pts = pts;
    c->last_update = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

void set_clock(Clock *c, double pts, int serial) {
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

double get_clock(Clock *c) {
    if (*c->queue_serial != c->serial) {
        return NAN;
    }
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time - (time - c->last_update) * (1.0 - c->speed);
    }
}

void set_clock_speed(Clock *c, double speed) {
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}

void sync_clock_to_slave(Clock *c, Clock *slave) {
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        set_clock(c, slave_clock, slave->serial);
    }
}

void init_clock(Clock *clock, int *serial) {
    clock->speed = 1.0;
    clock->paused = 0;
    clock->queue_serial = serial;
    set_clock(clock, NAN, -1);
}

int decode_interrupt_cb(void *ctx) {
    VideoState *is = ctx;
    return is->abort_request;
}

int get_master_sync_type(VideoState *is) {
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st) {
            return AV_SYNC_VIDEO_MASTER;
        } else {
            return AV_SYNC_AUDIO_MASTER;
        }
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st) {
            return AV_SYNC_AUDIO_MASTER;
        } else {
            return AV_SYNC_EXTERNAL_CLOCK;
        }
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}


double get_master_clock(VideoState *is) {
    double val;
    switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->video_clock);
            break;
            
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->sample_clock);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}

int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx, AVFilterContext *sink_ctx) {
    int ret;
    int nb_filters = graph->nb_filters;
    AVFilterInOut *outputs = NULL, *inputs = NULL;
    if (filtergraph) {
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            avfilter_inout_free(&outputs);
            avfilter_inout_free(&inputs);
            LOGE("avfilter_inout_alloc failed");
            return AVERROR(ENOMEM);
        }
        outputs->name = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;
        
        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;
        if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0) {
            avfilter_inout_free(&outputs);
            avfilter_inout_free(&inputs);
            av_err2str(ret);
            LOGE("avfilter_graph_parse_ptr failed: ret", ret);
            return ret;
        }
    } else {
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0) {
            avfilter_inout_free(&outputs);
            avfilter_inout_free(&inputs);
            av_err2str(ret);
            LOGE("avfilter_link failed: %d", ret);
            return ret;
        }
    }
    for (int i = 0; i < graph->nb_filters - nb_filters; i++) {
        FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);
    }
    ret = avfilter_graph_config(graph, NULL);
    if (ret < 0) {
        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
        av_err2str(ret);
        LOGE("avfilter_graph_config failed: %d message: %s", ret, av_err2str(ret));
        return ret;
    }
    return ret;
}

int configure_audio_filters(VideoState *is, const char *afilters, int force_outout_format) {
    int ret;
    static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    int sample_rates[2] = { 0, -1 };
    int64_t channel_layouts[2] = { 0, -1 };
    int channels[2] = { 0, -1 };
    AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
    char asrc_args[256];
    
    avfilter_graph_free(&is->agraph);
    if (!(is->agraph = avfilter_graph_alloc())) {
        LOGE("avfilter_graph_alloc");
        return AVERROR(ENOMEM);
    }
    ret = snprintf(asrc_args, sizeof(asrc_args), "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                   is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
                   is->audio_filter_src.channels, 1, is->audio_filter_src.freq);
    if (is->audio_filter_src.channel_layout) {
        snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%"PRIx64, is->audio_filter_src.channel_layout);
    }
    ret = avfilter_graph_create_filter(&filt_asrc, avfilter_get_by_name("abuffer"), "in", asrc_args, NULL, is->agraph);
    if (ret < 0) {
        LOGE("avfilter_graph_create_filter abuffer failed: %d", ret);
        avfilter_graph_free(&is->agraph);
        av_err2str(ret);
        return ret;
    }
    ret = avfilter_graph_create_filter(&filt_asink, avfilter_get_by_name("abuffersink"), "out", NULL, NULL, is->agraph);
    if (ret < 0) {
        LOGE("avfilter_graph_create_filter abuffersink failed: %d", ret);
        avfilter_graph_free(&is->agraph);
        av_err2str(ret);
        return ret;
    }
    if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0) {
        LOGE("av_opt_set_int_list sample_fmts failed: %d", ret);
        avfilter_graph_free(&is->agraph);
        av_err2str(ret);
        return ret;
    }
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        LOGE("av_opt_set_int all_channel_counts failed: %d", ret);
        avfilter_graph_free(&is->agraph);
        av_err2str(ret);
        return ret;
    }
    if (force_outout_format) {
        channel_layouts[0] = is->audio_filter_src.channel_layout;
        channels[0] = is->audio_filter_src.channels;
        sample_rates[0] = is->audio_filter_src.freq;
        if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
            LOGE("av_opt_set_int_list channel_layout failed: %d", ret);
            avfilter_graph_free(&is->agraph);
            av_err2str(ret);
            return ret;
        }
        if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0) {
            LOGE("av_opt_set_int_list all_channel_counts failed: %d", ret);
            avfilter_graph_free(&is->agraph);
            av_err2str(ret);
            return ret;
        }
        if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
            LOGE("av_opt_set_int_list channel_counts failed: %d", ret);
            avfilter_graph_free(&is->agraph);
            av_err2str(ret);
            return ret;
        }
        if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
            LOGE("av_opt_set_int_list sample_rates failed: %d", ret);
            avfilter_graph_free(&is->agraph);
            av_err2str(ret);
            return ret;
        }
    }
    if ((ret = configure_filtergraph(is->agraph, afilters, filt_asrc, filt_asink)) < 0) {
        LOGE("configure_filtergraph error: %d args: %s", ret, asrc_args);
        avfilter_graph_free(&is->agraph);
        return ret;
    }
    is->in_audio_filter = filt_asrc;
    is->out_audio_filter = filt_asink;
    return ret;
}

#ifdef __APPLE__
/* copy samples for viewing in editor window */
void update_sample_display(VideoState *is, short *samples, int sample_size) {
    int size, len;
    size = sample_size / sizeof(short);
    while (size > 0) {
        len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
        if (len > size)
            len = size;
        memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
        samples += len;
        is->sample_array_index += len;
        if (is->sample_array_index >= SAMPLE_ARRAY_SIZE) {
            is->sample_array_index = 0;
        }
        size -= len;
    }
}
#endif

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
int synchronize_audio(VideoState *is, int nb_samples) {
    int wanted_nb_samples = nb_samples;
    
    /* if not master, then we try to remove or add samples to correct the clock */
    if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {
        double diff, avg_diff;
        int min_nb_samples, max_nb_samples;
        
        diff = get_clock(&is->sample_clock) - get_master_clock(is);
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                /* not enough measures to have a correct estimate */
                is->audio_diff_avg_count++;
            } else {
                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
                
                if (fabs(avg_diff) >= is->audio_diff_threshold) {
                    wanted_nb_samples = nb_samples + (int) (diff * is->audio_src.freq);
                    min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    wanted_nb_samples = av_clip_c(wanted_nb_samples, min_nb_samples, max_nb_samples);
                }
                av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
                       diff, avg_diff, wanted_nb_samples - nb_samples,
                       is->audio_clock, is->audio_diff_threshold);
            }
        } else {
            /* too big difference : may be initial PTS errors, so reset A-V filter */
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum = 0;
        }
    }
    return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
int audio_decoder_frame(VideoState *is) {
    int data_size, resample_data_size;
    int64_t dec_channel_layout;
    av_unused double audio_clock0;
    int wanted_nb_sample;
    Frame *af;

    if (is->paused) {
        LOGE("ffplay paused");
        return -1;
    }
    do {
        if (!(af = frame_queue_peek_readable(&is->sample_queue))) {
            LOGE("frame_queue_peek_readable");
            return -1;
        }
        frame_queue_next(&is->sample_queue);
    } while (af->serial != is->audioq.serial);

    data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame), af->frame->nb_samples,
                                           af->frame->format, 1);
    
    dec_channel_layout = (af->frame->channel_layout && av_frame_get_channels(af->frame) == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
                            af->frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(af->frame));
    wanted_nb_sample = synchronize_audio(is, af->frame->nb_samples);
    
    if (af->frame->format != is->audio_src.fmt ||
        dec_channel_layout != is->audio_src.channel_layout ||
        af->frame->sample_rate != is->audio_src.freq ||
        (wanted_nb_sample != af->frame->nb_samples && !is->swr_ctx)) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL, is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
                                         dec_channel_layout, af->frame->format, af->frame->sample_rate, 0, NULL);
        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format), av_frame_get_channels(af->frame),
                   is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
            return -1;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels = av_frame_get_channels(af->frame);
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = af->frame->format;
    }
    if (is->swr_ctx) {
        const uint8_t **in = (const uint8_t **) af->frame->extended_data;
        uint8_t **out = &is->audio_buf1;
        int out_count = wanted_nb_sample * is->audio_tgt.freq / af->frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
             av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_sample != af->frame->nb_samples) {
            if (swr_set_compensation(is->swr_ctx, (wanted_nb_sample - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
                                     wanted_nb_sample * is->audio_tgt.freq / af->frame->sample_rate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) {
            return AVERROR(ENOMEM);
        }
        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too samll.\n");
            if (swr_init(is->swr_ctx) < 0) {
                swr_free(&is->swr_ctx);
            }
        }
        is->audio_buf = is->audio_buf1;
        resample_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
    } else {
        is->audio_buf = af->frame->data[0];
        resample_data_size = data_size;
    }
    audio_clock0 = is->audio_clock;
    
    /* update the audio clock with the pts */
    if (!isnan(af->pts)) {
        is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    } else {
        is->audio_clock = NAN;
    }
    
#ifdef DEBUG
    {
//        static double last_clock;
//        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
//               is->audio_clock - last_clock,
//               is->audio_clock, audio_clock0);
//        last_clock = is->audio_clock;
    }
#endif
    return resample_data_size;
}

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    MyAVPacketList *pkt1;
    int ret;
    pthread_mutex_lock(&q->mutex);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt) {
                q->last_pkt = NULL;
            }
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt = pkt1->pkt;
            if (serial) {
                *serial = pkt1->serial;
            }
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    int ret;
    pthread_mutex_lock(&q->mutex);
    ret = packet_queue_put_private(q, pkt);
    pthread_mutex_unlock(&q->mutex);
    if (pkt != &flush_pkt && ret < 0) {
        av_packet_unref(pkt);
    }
    return ret;
}

int packet_queue_put_nullpacket(PacketQueue *q, int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

int decoder_decode_frame(VideoState* is, Decoder *d, AVFrame *frame, AVSubtitle *sub) {
    int got_frame = 0;
    int64_t time = 0;
    do {
        int ret = -1;
        if (d->queue->abort_request) {
            return -1;
        }
        if (!d->packet_pending || d->queue->serial != d->pkt_serial) {
            AVPacket pkt;
            do {
                if (d->queue->nb_packets == 0) {
                    pthread_cond_signal(&d->empty_queue_cond);
                }
                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0) {
                    return -1;
                }
                if (pkt.data == flush_pkt.data) {
                    avcodec_flush_buffers(d->codec_context);
                    d->finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                }
            } while (pkt.data == flush_pkt.data || d->queue->serial != d->pkt_serial);
            av_packet_unref(&d->pkt);
            d->pkt_temp = d->pkt = pkt;
            d->packet_pending = 1;
        }
        
        switch (d->codec_context->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                ret = avcodec_decode_video2(d->codec_context, frame, &got_frame, &d->pkt_temp);
                if (got_frame) {
                    frame->pts = frame->pkt_dts;
                    time = (int64_t) (d->pkt_temp.pts * av_q2d(is->video_st->time_base) * 1000);
                    // 播放到指定时间
                    is->finish = time > is->end_time;
                }
                break;
                
            case AVMEDIA_TYPE_AUDIO:
                ret = avcodec_decode_audio4(d->codec_context, frame, &got_frame, &d->pkt_temp);
                if (got_frame) {
                    AVRational tb = (AVRational) { 1, frame->sample_rate };
                    if (frame->pts != AV_NOPTS_VALUE) {
                        frame->pts = av_rescale_q(frame->pts, d->codec_context->time_base, tb);
                    } else if (frame->pkt_pts != AV_NOPTS_VALUE) {
                        frame->pts = av_rescale_q(frame->pkt_pts, av_codec_get_pkt_timebase(d->codec_context), tb);
                    } else if (d->next_pts != AV_NOPTS_VALUE) {
                        frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                    }
                    if (frame->pts != AV_NOPTS_VALUE) {
                        d->next_pts = frame->pts + frame->nb_samples;
                        d->next_pts_tb = tb;
                    }
                    time = (int64_t) (d->pkt_temp.pts * av_q2d(is->audio_st->time_base) * 1000);
                }
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                ret = avcodec_decode_subtitle2(d->codec_context, sub, &got_frame, &d->pkt_temp);
                break;
            default:
                break;
        }
        if (ret < 0) {
            d->packet_pending = 0;
        } else {
            d->pkt_temp.dts = d->pkt_temp.pts = AV_NOPTS_VALUE;
            if (d->pkt_temp.data) {
                if (d->codec_context->codec_type != AVMEDIA_TYPE_AUDIO) {
                    ret = d->pkt_temp.size;
                }
                d->pkt_temp.data += ret;
                d->pkt_temp.size -= ret;
                if (d->pkt_temp.size <= 0) {
                    d->packet_pending = 0;
                }
            } else {
                if (!got_frame) {
                    d->packet_pending = 0;
                    d->finished = d->pkt_serial;
                }
            }
        }

    } while ((!got_frame && !d->finished) || (time < is->seek_pos && is->precision_seek));
    return got_frame;
}

void decoder_release(Decoder *d) {
    av_packet_unref(&d->pkt);
    avcodec_free_context(&d->codec_context);
}

void decoder_abort(Decoder *d, FrameQueue *fq) {
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    pthread_join(d->decoder_tid, NULL);
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}

int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels) {
        return channel_layout;
    } else {
        return 0;
    }
}

int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1, enum AVSampleFormat fmt2, int64_t channel_count2) {
    if (channel_count1 == 1 && channel_count2 == 1) {
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    } else {
        return channel_count1 != channel_count2 || fmt1 != fmt2;
    }
}

void* audio_thread(void *arg) {
    VideoState *is = arg;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
    int last_serial = -1;
    int64_t dec_channel_layout;
    int reconfigure;
    int got_frame;
    AVRational tb;
    int ret = 0;
    if (!frame) {
        return NULL;
    }
    do {
        if ((got_frame = decoder_decode_frame(is, &is->audio_decode, frame, NULL)) < 0) {
            LOGE("decoder_decode_frame: %d", got_frame);
            avfilter_graph_free(&is->agraph);
            av_frame_free(&frame);
            return NULL;
        }
        if (got_frame) {
            tb = (AVRational) { 1, frame->sample_rate };
            dec_channel_layout = get_valid_channel_layout(frame->channel_layout, av_frame_get_channels(frame));
            reconfigure = cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels, frame->format, av_frame_get_channels(frame));
            if (reconfigure) {
                char buf1[1024], buf2[1024];
                av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
                av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
                av_log(NULL, AV_LOG_DEBUG,
                       "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
                       is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
                       frame->sample_rate, av_frame_get_channels(frame), av_get_sample_fmt_name(frame->format), buf2, is->audio_decode.pkt_serial);
                is->audio_filter_src.fmt = frame->format;
                is->audio_filter_src.channel_layout = dec_channel_layout;
                is->audio_filter_src.channels = av_frame_get_channels(frame);
                is->audio_filter_src.freq = frame->sample_rate;
                last_serial = is->audio_decode.pkt_serial;
                if ((ret = configure_audio_filters(is, NULL, 1)) < 0) {
                    avfilter_graph_free(&is->agraph);
                    av_frame_free(&frame);
                    return NULL;
                }
            }
            if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0) {
                LOGE("av_buffersrc_add_frame error: %s", av_err2str(ret));
                avfilter_graph_free(&is->agraph);
                av_frame_free(&frame);
                av_err2str(ret);
                return NULL;
            }
            while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
                tb = is->out_audio_filter->inputs[0]->time_base;
                if (!(af = frame_queue_peek_writable(&is->sample_queue))) {
                    avfilter_graph_free(&is->agraph);
                    av_frame_free(&frame);
                    return NULL;
                }
                af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
                af->pos = av_frame_get_pkt_pos(frame);
                af->serial = is->audio_decode.pkt_serial;
                af->duration = av_q2d((AVRational) { frame->nb_samples, frame->sample_rate });
                av_frame_move_ref(af->frame, frame);
                frame_queue_push(&is->sample_queue);
                if (is->audioq.serial != is->audio_decode.pkt_serial) {
                    break;
                }
            }
            if (ret == AVERROR_EOF) {
                is->audio_decode.finished = is->audio_decode.pkt_serial;
            }
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    return NULL;
}

int get_video_frame(VideoState *is, AVFrame *frame) {
    int got_picture;
    if ((got_picture = decoder_decode_frame(is, &is->video_decode, frame, NULL)) < 0) {
        return got_picture;
    }
    if (got_picture) {
        double dpts = NAN;
        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(is->video_st->time_base) * frame->pts;
        }
        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
        is->viddec_width = frame->width;
        is->viddec_height = frame->height;
    }
    return got_picture;
}

int configure_video_filters(AVFilterGraph *graph, VideoState *is, const char *vfilters, AVFrame *frame) {
    int ret;
    static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    char sws_flags_str[512] = "";
    char buffersrc_args[256] = "";
    AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
    AVCodecParameters *codecpar = is->video_st->codecpar;
    AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
    av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", "bicubic");
    if (strlen(sws_flags_str)) {
        sws_flags_str[strlen(sws_flags_str) - 1] = '\0';
    }
    graph->scale_sws_opts = av_strdup(sws_flags_str);
    snprintf(buffersrc_args, sizeof(buffersrc_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width, frame->height, frame->format, is->video_st->time_base.num, is->video_st->time_base.den,
             codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
    if (fr.num && fr.den) {
        av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);
    }
    if ((ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"), "ffplay_buffer", buffersrc_args, NULL, graph)) < 0) {
        char* error = av_err2str(ret);
        return ret;
    }
    if ((ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"), "ffplay_buffersink", NULL, NULL, graph)) < 0) {
        char* error = av_err2str(ret);
        return ret;
    }
    if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0) {
        char* error = av_err2str(ret);
        return ret;
    }
    last_filter = filt_out;
    if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0) {
        return ret;
    }
    is->in_video_filter = filt_src;
    is->out_video_filter = filt_out;
    return ret;
}

int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial) {
    Frame *vp;
    if (!(vp = frame_queue_peek_writable(&is->video_queue))) {
        return -1;
    }
    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;
    
#ifdef __APPLE__
    if (!vp->bmp || !vp->allocated ||
        vp->width != src_frame->width ||
        vp->height != src_frame->height ||
        vp->format != src_frame->format) {
        SDL_Event event;
        vp->allocated = 0;
        vp->reallocated = 0;
        vp->width = src_frame->width;
        vp->height = src_frame->height;
        
        /* the allocation must be done in the main thread to avoid
         locking problems. */
        event.type = FF_ALLOC_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
        
        /* wait until the picture is allocated */
        pthread_mutex_lock(&is->video_queue.mutex);
        while (!vp->allocated && !is->videoq.abort_request) {
            pthread_cond_wait(&is->video_queue.cond, &is->video_queue.mutex);
        }
        /* if the queue is aborted, we have to pop the pending ALLOC event or wait for the allocation to complete */
        //TODO 2.0
        if (is->videoq.abort_request && SDL_PeepEvents(&event, 1, SDL_GETEVENT, FF_ALLOC_EVENT, FF_ALLOC_EVENT) != 1) {
            while (!vp->allocated && !is->abort_request) {
                pthread_cond_wait(&is->video_queue.cond, &is->video_queue.mutex);
            }
        }
        pthread_mutex_unlock(&is->video_queue.mutex);
        if (is->videoq.abort_request) {
            return -1;
        }
    }
    /* if the frame is not skipped, then display it */
    if (vp->bmp) {
        vp->pts = pts;
        vp->duration = duration;
        vp->pos = pos;
        vp->serial = serial;
        
        av_frame_move_ref(vp->frame, src_frame);
        frame_queue_push(&is->video_queue);
    }
#else
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;
    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->video_queue);
#endif
    return 0;
}

void* video_thread(void *arg) {
    int ret;
    VideoState *is = arg;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);
    AVFilterGraph *graph = avfilter_graph_alloc();
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = -2;
    int last_serial = -1;
    int last_vfilter_idx = 0;
    if (!graph) {
        av_frame_free(&frame);
        return NULL;
    }
    if (!frame) {
        avfilter_graph_free(&graph);
        return NULL;
    }
    while(1) {
        ret = get_video_frame(is, frame);
        if (ret < 0) {
            avfilter_graph_free(&graph);
            av_frame_free(&frame);
            return 0;
        }
        if (!ret) {
            continue;
        }
        if (last_w != frame->width || last_h != frame->height
            || last_format != frame->format
            || last_serial != is->video_decode.pkt_serial
            || last_vfilter_idx != is->vfilter_idx) {
            av_log(NULL, AV_LOG_DEBUG,
                   "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
                   last_w, last_h,
                   (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
                   frame->width, frame->height,
                   (const char *)av_x_if_null(av_get_pix_fmt_name((enum AVPixelFormat) frame->format), "none"), is->video_decode.pkt_serial);
            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if ((ret = configure_video_filters(graph, is, NULL, frame)) < 0) {
#ifdef __APPLE__
                SDL_Event event;
                event.type = FF_QUIT_EVENT;
                event.user.data1 = is;
                SDL_PushEvent(&event);
#endif
                avfilter_graph_free(&graph);
                av_frame_free(&frame);
                return NULL;
            }
            filt_in = is->in_video_filter;
            filt_out = is->out_video_filter;
            last_w = frame->width;
            last_h = frame->height;
            last_format = frame->format;
            last_serial = is->video_decode.pkt_serial;
            last_vfilter_idx = is->vfilter_idx;
            frame_rate = filt_out->inputs[0]->frame_rate;
        }
        
        ret = av_buffersrc_add_frame(filt_in, frame);
        if (ret < 0) {
            av_err2str(ret);
            avfilter_graph_free(&graph);
            av_frame_free(&frame);
            return NULL;
        }
        while (ret >= 0) {
            is->frame_last_returned_time = av_gettime_relative() / 1000000.0;
            ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    is->video_decode.finished = is->video_decode.pkt_serial;
                }
                ret = 0;
                break;
            }
            is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0) {
                is->frame_last_filter_delay = 0;
            }
            tb = filt_out->inputs[0]->time_base;
            duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational) { frame_rate.den, frame_rate.num }) : 0);
            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            ret = queue_picture(is, frame, pts, duration, av_frame_get_pkt_pos(frame), is->video_decode.pkt_serial);
            av_frame_unref(frame);
        }
    }
    return NULL;
}

void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, pthread_cond_t empty_queue_cond) {
    memset(d, 0, sizeof(Decoder));
    d->codec_context = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
}

int decoder_start(Decoder *d, void* (*fn) (void*), void *arg) {
    packet_queue_start(d->queue);
    int result = pthread_create(&d->decoder_tid, NULL, fn, arg);
    if (result != 0) {
        av_log(NULL, AV_LOG_ERROR, "create decode thread failed: %d\n", result);
        return AVERROR(ENOMEM);
    }
    return 0;
}

int stream_component_open(VideoState *is, int stream_index) {
    int ret;
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    AVDictionary *opts = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int stream_lowres = lowres;
    if (stream_index < 0 || stream_index >= ic->nb_streams) {
        return -1;
    }
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        return AVERROR(ENOMEM);
    }
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0) {
        av_err2str(ret);
        avcodec_free_context(&avctx);
        return ret;
    }
    
    av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);
    codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n", avctx->codec_id);
        avcodec_free_context(&avctx);
        return AVERROR(ENOMEM);
    }
    avctx->codec_id = codec->id;
    if (stream_lowres > av_codec_get_max_lowres(codec)) {
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
               av_codec_get_max_lowres(codec));
        stream_lowres = av_codec_get_max_lowres(codec);
    }
    av_codec_set_lowres(avctx, stream_lowres);
    
#if FF_API_EMU_EDGE
    if(stream_lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif
    if (is->fast) {
        avctx->flags |= AV_CODEC_FLAG2_FAST;
    }
#if FF_API_EMU_EDGE
    if (codec->capabilities & AV_CODEC_CAP_DR1) {
        avctx->flags |= CODEC_FLAG_EMU_EDGE;
    }
#endif
    av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres) {
        av_dict_set_int(&opts, "lowers", stream_lowres, 0);
    }
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    }
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        avcodec_free_context(&avctx);
        av_err2str(ret);
        return ret;
    }
    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO: {
            AVFilterLink *link;
            is->audio_filter_src.freq = avctx->sample_rate;
            is->audio_filter_src.channels = avctx->channels;
            is->audio_filter_src.channel_layout = avctx->channel_layout;
            is->audio_filter_src.fmt = avctx->sample_fmt;
            if ((ret = configure_audio_filters(is, NULL, 0)) < 0) {
                avcodec_free_context(&avctx);
                LOGE("configure_audio_filters failed: %d", ret);
                return ret;
            }
            link = is->out_audio_filter->inputs[0];
            sample_rate = link->sample_rate;
            channel_layout = link->channel_layout;
            nb_channels = link->channels;

            /* prepare audio output */
//            if (is->audio_render_context) {
//                ret = is->audio_render_context->InitAudio(is->audio_render_context,
//                        channel_layout, nb_channels, sample_rate, &is->audio_tgt);
//                if (ret < 0) {
//                    LOGE("Init audio render error: %d", ret);
//                    avcodec_free_context(&codec_context);
//                    return ret;
//                }
//            }
//            LOGE("AVMEDIA_TYPE_AUDIO");
            is->audio_hw_buf_size = ret;
            is->audio_tgt.fmt = AV_SAMPLE_FMT_S16;
            is->audio_tgt.freq = 44100;
            is->audio_tgt.channel_layout = AV_CH_LAYOUT_MONO;
            is->audio_tgt.channels = 1;
            is->audio_tgt.frame_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, 1, is->audio_tgt.fmt, 1);
            is->audio_tgt.bytes_per_sec = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, is->audio_tgt.freq, is->audio_tgt.fmt, 1);
            if (is->audio_tgt.bytes_per_sec <= 0 || is->audio_tgt.frame_size <= 0) {
                return -1;
            }
            is->audio_src = is->audio_tgt;
            is->audio_buf_size = 0;
            is->audio_buf_index = 0;
            
            /* init averaging filter */
            is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
            is->audio_diff_avg_count = 0;
            
            /* since we do not have a precise anough audio fifo fullness,
             we correct audio sync only if larger than this threshold */
            is->audio_diff_threshold = (double) (is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;
            is->audio_stream = stream_index;
            is->audio_st = ic->streams[stream_index];
            
            decoder_init(&is->audio_decode, avctx, &is->audioq, is->continue_read_thread);
            if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
                is->audio_decode.start_pts = is->audio_st->start_time;
                is->audio_decode.start_pts_tb = is->audio_st->time_base;
            }
            if ((ret = decoder_start(&is->audio_decode, audio_thread, is)) < 0) {
                avcodec_free_context(&avctx);
                return ret;
            }
#ifdef __APPLE__
            SDL_PauseAudio(0);
#endif
        }
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_stream = stream_index;
            is->video_st = ic->streams[stream_index];
            is->viddec_width = avctx->width;
            is->viddec_height = avctx->height;
            decoder_init(&is->video_decode, avctx, &is->videoq, is->continue_read_thread);
            if ((ret = decoder_start(&is->video_decode, video_thread, is)) < 0) {
                avcodec_free_context(&avctx);
                return ret;
            }
            is->queue_attachments_req = 1;
            break;
            
        case AVMEDIA_TYPE_SUBTITLE:
            break;
        default:
            break;
    }
    return ret;
}

/* seek in the stream */
void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes) {
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes) {
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        }
        is->seek_req = 1;
        pthread_cond_signal(&is->continue_read_thread);
    }
}

/* pause or resume the video */
void stream_toggle_pause(VideoState *is) {
    if (is->paused) {
        is->frame_timer += av_gettime_relative() / 1000000.0 - is->video_clock.last_update;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->video_clock.paused = 0;
        }
        set_clock(&is->video_clock, get_clock(&is->video_clock), is->video_clock.serial);
    }
    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    pthread_mutex_lock(&is->render_mutex);
    is->paused = is->sample_clock.paused = is->video_clock.paused = is->extclk.paused = !is->paused;
    pthread_cond_signal(&is->render_cond);
    pthread_mutex_unlock(&is->render_mutex);
}

void step_to_next_frame(VideoState *is) {
    /* if the stream is paused unpause it, then step */
    if (is->paused) {
        stream_toggle_pause(is);
    }
    is->step = 1;
}

void read_thread_failed(VideoState *is, AVFormatContext *ic, pthread_mutex_t wait_mutex) {
    if (ic && !is->ic) {
        avformat_close_input(&ic);
    }
    
#ifdef __APPLE__
    SDL_Event event;
    event.type = FF_QUIT_EVENT;
    event.user.data1 = is;
    SDL_PushEvent(&event);
#endif
    pthread_mutex_destroy(&wait_mutex);
}

static void stream_component_close(VideoState *is, int stream_index) {
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    
    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    avctx = ic->streams[stream_index]->codec;
    
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            decoder_abort(&is->audio_decode, &is->sample_queue);
#ifdef __APPLE__
            SDL_CloseAudio();
#endif
            decoder_release(&is->audio_decode);
            swr_free(&is->swr_ctx);
            av_freep(&is->audio_buf1);
            is->audio_buf1_size = 0;
            is->audio_buf = NULL;
            
#ifdef __APPLE__
            if (is->rdft) {
                av_rdft_end(is->rdft);
                av_freep(&is->rdft_data);
                is->rdft = NULL;
                is->rdft_bits = 0;
            }
#endif
            break;
        case AVMEDIA_TYPE_VIDEO:
            decoder_abort(&is->video_decode, &is->video_queue);
            decoder_release(&is->video_decode);
            break;
        default:
            break;
    }
    
    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    avcodec_close(avctx);
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            is->audio_st = NULL;
            is->audio_stream = -1;
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_st = NULL;
            is->video_stream = -1;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
#ifdef __APPLE__
            is->subtitle_st = NULL;
#endif
            is->subtitle_stream = -1;
            break;
        default:
            break;
    }
}

void stream_close(VideoState *is) {
    if (!is->filename) {
        return;
    }
    pthread_mutex_lock(&is->render_mutex);
    is->paused = 0;
    pthread_cond_signal(&is->render_cond);
    pthread_mutex_unlock(&is->render_mutex);
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
    pthread_join(is->read_tid, NULL);
    pthread_join(is->render_thread, NULL);

    LOGE("enter stream close: %s", is->filename);
    pthread_mutex_destroy(&is->render_mutex);
    pthread_cond_destroy(&is->render_cond);
    /* close each stream */
    if (is->audio_stream >= 0)
        stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);
    if (is->subtitle_stream >= 0)
        stream_component_close(is, is->subtitle_stream);
    
    avformat_close_input(&is->ic);
    
    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);
    packet_queue_destroy(&is->subtitleq);
    
    /* free all pictures */
    frame_queue_destory(&is->video_queue);
    frame_queue_destory(&is->sample_queue);
    frame_queue_destory(&is->subtitle_queue);
    pthread_cond_destroy(&is->continue_read_thread);
#ifdef __APPLE__
    sws_freeContext(is->img_convert_ctx);
    sws_freeContext(is->sub_convert_ctx);
#endif
    av_free(is->filename);
    is->filename = NULL;
    LOGE("leave stream close");
}

void av_destroy(VideoState *is) {
    if (is) {
        stream_close(is);
    }
}

int complete_state(VideoState* is) {
    // TODO 会调用多次
    // 后面需要想办法优化
    if (is->state_context && is->state_context->complete) {
        return is->state_context->complete(is->state_context);
    }
    return 0;
}

void* read_thread(void *arg) {
    LOGE("enter read_thread ");
    VideoState *is = arg;
    AVFormatContext *ic = NULL;
    int ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    pthread_mutex_t wait_mutex;
    ret = pthread_mutex_init(&wait_mutex, NULL);
    int64_t pkt_ts;
    if (ret != 0) {
        av_log(NULL, AV_LOG_FATAL, "Read Thread mutex: %d\n", ret);
        read_thread_failed(is, NULL, wait_mutex);
        return NULL;
    }
    memset(st_index, -1, sizeof(st_index));
    is->eof = 0;
    ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Can't allocate context.\n");
        read_thread_failed(is, ic, wait_mutex);
        return NULL;
    }
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = is;
    ret = avformat_open_input(&ic, is->filename, NULL, NULL);
    if (ret < 0) {
        read_thread_failed(is, ic, wait_mutex);
        av_err2str(ret);
        return NULL;
    }
    is->ic = ic;
    av_format_inject_global_side_data(ic);
    
//    if ((ret = avformat_find_stream_info(ic, NULL)) < 0) {
//        read_thread_failed(is, ic, wait_mutex);
//        av_err2str(ret);
//        return NULL;
//    }
//    if (ic->pb) {
//        ic->pb->eof_reached = 0;
//    }
    
    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    
    /* 如果start_time != AV_NOPTS_VALUE,则从start_time位置开始播放 */
    if (is->start_time != 0) {
        int64_t timestamp = is->start_time;

        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE) {
            timestamp += ic->start_time;
        }
//        av_seek_frame(ic, -1, timestamp * (AV_TIME_BASE / 1000), AVSEEK_FLAG_BACKWARD);
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp * (AV_TIME_BASE / 1000), INT64_MAX, 0);
        if (ret < 0) {
            av_err2str(ret);
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                   is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }
    
    av_dump_format(ic, 0, is->filename, 0);
    
    for (int i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        if (wanted_stream_spec[type] && st_index[i] == -1) {
            if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0) {
                st_index[type] = i;
            }
        }
        
    }
    for (int i = 0; i < AVMEDIA_TYPE_NB; i++) {
        if (wanted_stream_spec[i] && st_index[i] == -1) {
            av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", wanted_stream_spec[i], av_get_media_type_string(i));
            st_index[i] = INT_MAX;
        }
    }
    st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO],
                                                       st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
    st_index[AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE, st_index[AVMEDIA_TYPE_SUBTITLE],
                                                          (st_index[AVMEDIA_TYPE_AUDIO] > 0 ? st_index[AVMEDIA_TYPE_AUDIO] : st_index[AVMEDIA_TYPE_VIDEO]),
                                                           NULL, 0);
    
    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
    }
    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }
    if (is->video_stream < 0 && is->audio_stream < 0) {
        read_thread_failed(is, ic, wait_mutex);
        return NULL;
    }
    while (!is->abort_request) {
//        if (is->abort_request) {
//            LOGE("is->abort_request");
//            break;
//        }
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused) {
                is->read_pause_return = av_read_pause(ic);
            } else {
                av_read_play(ic);
            }
        }
        if (is->seek_req) {
            int64_t seek_target = is->seek_pos * (AV_TIME_BASE / 1000);
            int64_t seek_min = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
            int64_t seek_max = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
            
            // FIXME the +-2 is due to rounding being not done in the correct direction in generation
            //      of the seek_pos/seek_rel variables
//            ret = av_seek_frame(ic, -1,  seek_target * (AV_TIME_BASE / 1000), AVSEEK_FLAG_BACKWARD);
            ret = avformat_seek_file(ic, -1, seek_min, seek_target, seek_max,  0 & (~AVSEEK_FLAG_BYTE));
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", is->filename);
            } else {
                if (is->audio_stream >= 0) {
                    packet_queue_flush(&is->audioq);
                    packet_queue_put(&is->audioq, &flush_pkt);
                }
                if (is->video_stream >= 0) {
                    packet_queue_flush(&is->videoq);
                    packet_queue_put(&is->videoq, &flush_pkt);
                }
                if (is->subtitle_stream >= 0) {
                    packet_queue_flush(&is->subtitleq);
                    packet_queue_put(&is->subtitleq, &flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                    set_clock(&is->extclk, NAN, 0);
                } else {
                    set_clock(&is->extclk, seek_target / (double) AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused) {
                step_to_next_frame(is);
            }
        }
        if (is->queue_attachments_req) {
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket copy;
                if ((ret = av_copy_packet(&copy, &is->video_st->attached_pic)) < 0) {
                    read_thread_failed(is, ic, wait_mutex);
                }
                packet_queue_put(&is->videoq, &copy);
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }
        
        /* if the queue are full, no need to read more */
        if ((is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE ||
             ((is->audioq.nb_packets > MIN_FRAMES || is->audio_stream < 0 || is->audioq.abort_request) &&
             (is->videoq.nb_packets > MIN_FRAMES || is->video_stream < 0 || is->videoq.abort_request ||
             (is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) &&
             (is->subtitleq.nb_packets > MIN_FRAMES || is->subtitle_stream < 0 || is->subtitleq.abort_request)))) {
            /* wait 10 ms */
            pthread_mutex_lock(&wait_mutex);
//            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            pthread_mutex_unlock(&wait_mutex);
            continue;
        }
        if (!is->paused &&
            (!is->audio_st || (is->audio_decode.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sample_queue) == 0)) &&
            (!is->video_st || (is->video_decode.finished == is->videoq.serial && frame_queue_nb_remaining(&is->video_queue) == 0))) {
            is->paused = 0;
            is->audio_decode.finished = 0;
            is->video_decode.finished = 0;
            int exit = complete_state(is);
            // 是否退出
            if (exit) {
                LOGE("player finish exit");
                return NULL;
            }
//            // loop and exit
//            if (is->loop) {
//                stream_seek(is, is->start_time != 0 ? is->start_time : 0, 0, 0);
//            } else {
//                av_destroy(is);
//                return NULL;
//            }
        }
        // 播放到指定时间
        // 如果是循环播放 seek到开始时间
        if (is->finish) {
            is->finish = 0;
            int exit = complete_state(is);
            if (exit) {
                LOGE("complete state exit");
                return NULL;
            }
        }
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0) {
                    packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                }
                if (is->audio_stream >= 0) {
                    packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                }
                if (is->subtitle_stream >= 0) {
                    packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
                }
                is->eof = 1;
            }
            if (ic->pb && ic->pb->error) {
                break;
            }
            continue;
        } else {
            is->eof = 0;
        }
        /* check if packet is in play range specified by user, then queue, otherwise discard */
        stream_start_time = ic->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        pkt_in_play_range = duration == AV_NOPTS_VALUE || (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                            av_q2d(ic->streams[pkt->stream_index]->time_base) -
                            (double)(is->start_time != 0 ? is->start_time : 0) / 1000000 <= ((double) duration / 1000000);
        if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream && pkt_in_play_range && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            packet_queue_put(&is->videoq, pkt);
        } else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
            packet_queue_put(&is->subtitleq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }
    LOGE("read thread exit");
    pthread_exit(0);
}

int stream_open(VideoState* is, const char *filename) {
    LOGE("stream open: %s", filename);
    is->filename = av_strdup(filename);
    if (!is->filename) {
        stream_close(is);
        return -1;
    }
    if (frame_queue_init(&is->video_queue, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0) {
        stream_close(is);
        return -1;
    }
    if (frame_queue_init(&is->subtitle_queue, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0) {
        stream_close(is);
        return -1;
    }
    if (frame_queue_init(&is->sample_queue, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0) {
        stream_close(is);
        return -1;
    }
    if (packet_queue_init(&is->videoq) < 0 ||
        packet_queue_init(&is->audioq) < 0 ||
        packet_queue_init(&is->subtitleq) < 0) {
        stream_close(is);
        return -1;
    }
    pthread_cond_init(&is->continue_read_thread, NULL);
    init_clock(&is->video_clock, &is->videoq.serial);
    init_clock(&is->sample_clock, &is->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);
    is->av_sync_type = av_sync_type;
    int result = pthread_create(&is->read_tid, NULL, read_thread, is);
    if (result != 0) {
        stream_close(is);
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %d\n", result);
        return -1;
    }
    return 0;
}

void check_external_clock_speed(VideoState *is) {
    if ((is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) ||
        (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)) {
        set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
        set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
        double speed = is->extclk.speed;
        if (speed != 1.0) {
            set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
        }
    }
}

static void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
    /* update current video pts */
    set_clock(&is->video_clock, pts, serial);
    sync_clock_to_slave(&is->extclk, &is->video_clock);
}

double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration) {
            return vp->duration;
        } else {
            return duration;
        }
    } else {
        return 0.0;
    }
}

double compute_target_delay(double delay, VideoState *is) {
    double sync_threshold, diff = 0;
    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
         duplicating or deleting a frame */
        diff = get_clock(&is->video_clock) - get_master_clock(is);

        /* skip or repeat frame. We take into account the
         delay to compute the threshold. I still don't know
         if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }
    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);
    return delay;
}

void video_display(VideoState *is) {
    // display video frame
    // TODO 播放结束时不能在发送渲染消息了
    if (NULL != is->video_render_context && is->video_queue.size > 0) {
        is->video_render_context->render_video_frame(is->video_render_context);
    }
}

void video_refresh(void *opaque, double *remaining_time) {
    VideoState *is = opaque;
    double time;
    if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK)
        check_external_clock_speed(is);

    if (is->audio_st) {
        time = av_gettime_relative() / 1000000.0;
        if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
            video_display(is);
            is->last_vis_time = time;
        }
        *remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
    }

    if (is->video_st) {
        retry:
        if (frame_queue_nb_remaining(&is->video_queue) == 0) {
            // nothing to do, no picture to display in the queue
        } else {
            double last_duration, delay;
            Frame *vp, *lastvp;

            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->video_queue);
            vp = frame_queue_peek(&is->video_queue);

            if (vp->serial != is->videoq.serial) {
                frame_queue_next(&is->video_queue);
                goto retry;
            }

            if (lastvp->serial != vp->serial)
                is->frame_timer = av_gettime_relative() / 1000000.0;

            if (is->paused)
                goto display;

            /* compute nominal last_duration */
            last_duration = vp_duration(is, lastvp, vp);
            delay = compute_target_delay(last_duration, is);

            time= av_gettime_relative()/1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                goto display;
            }

            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->frame_timer = time;

            pthread_mutex_lock(&is->video_queue.mutex);
            if (!isnan(vp->pts))
                update_video_pts(is, vp->pts, vp->pos, vp->serial);
            pthread_mutex_unlock(&is->video_queue.mutex);

            frame_queue_next(&is->video_queue);
            is->force_refresh = 1;

            if (is->step && !is->paused)
                stream_toggle_pause(is);
        }
        display:
        /* display picture */
        if (is->force_refresh && is->video_queue.rindex_shown)
            video_display(is);
    }
    is->force_refresh = 0;
}

void* render_thread(void* context) {
    VideoState* is = context;
    double remaining_time = 0.0;
    while (!is->abort_request) {
        pthread_mutex_lock(&is->render_mutex);
        if (is->paused) {
            pthread_cond_wait(&is->render_cond, &is->render_mutex);
        }
        pthread_mutex_unlock(&is->render_mutex);
        if (remaining_time > 0.0) {
            av_usleep((int64_t)(remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;
        if (!is->paused || is->force_refresh) {
            video_refresh(is, &remaining_time);
        }
    }
    LOGE("render thread exit");
    pthread_exit(0);
}

int av_start(VideoState* is, const char* file_name) {
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;
    int result = stream_open(is, file_name);
    pthread_mutex_init(&is->render_mutex, NULL);
    pthread_cond_init(&is->render_cond, NULL);
    pthread_create(&is->render_thread, NULL, render_thread, is);
    return result;
}

void av_seek(VideoState* is, uint64_t time) {
    stream_seek(is, time, 0, 0);
}
