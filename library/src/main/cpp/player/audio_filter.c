//
//  audio_filter.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//
#include <pthread.h>
#include <libavutil/opt.h>
#include "audio_filter.h"

AudioFilterContext *audio_filter_create() {
    AudioFilterContext* audio_filter_context = malloc(sizeof(AudioFilterContext));
    audio_filter_context->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(audio_filter_context->mutex, NULL);
    audio_filter_context->filter_graph = NULL;
    audio_filter_context->buffer_src_context = NULL;
    audio_filter_context->buffer_sink_context = NULL;
    audio_filter_context->abuffer_src = avfilter_get_by_name("abuffer");
    audio_filter_context->abuffer_sink = avfilter_get_by_name("abuffersink");
    return audio_filter_context;
}

void audio_filter_release(AudioFilterContext *audio_filter_context) {
    if (audio_filter_context->buffer_src_context != NULL) {
        avfilter_free(audio_filter_context->buffer_src_context);
    }
    if (audio_filter_context->buffer_sink_context != NULL) {
        avfilter_free(audio_filter_context->buffer_sink_context);
    }
    if (audio_filter_context->filter_graph != NULL) {
        avfilter_graph_free(&audio_filter_context->filter_graph);
    }
    pthread_mutex_destroy(audio_filter_context->mutex);
    free(audio_filter_context->mutex);
    free(audio_filter_context);
}

void audio_filter_change_speed(AVPlayContext *context, float speed) {
    AudioFilterContext *audio_filter_context = context->audio_filter_context;

    LOGE("audio_filter_change_speed: %f", speed);
    pthread_mutex_lock(audio_filter_context->mutex);
    if (audio_filter_context->buffer_src_context != NULL) {
        avfilter_free(audio_filter_context->buffer_src_context);
    }
    if (audio_filter_context->buffer_sink_context != NULL) {
        avfilter_free(audio_filter_context->buffer_sink_context);
    }
    if (audio_filter_context->filter_graph != NULL) {
        avfilter_graph_free(&audio_filter_context->filter_graph);
    }
    audio_filter_context->inputs = avfilter_inout_alloc();
    audio_filter_context->outputs = avfilter_inout_alloc();
    audio_filter_context->filter_graph = avfilter_graph_alloc();
    AVRational time_base = context->format_context->streams[context->audio_index]->time_base;
    AVCodecContext *dec_ctx = context->audio_codec_context;
    enum AVSampleFormat out_sample_fmts[] = {AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
    int out_sample_rates[] = {0, -1};
    out_sample_rates[0] = context->sample_rate;
    int out_sample_channel_count[] = {1, -1};
    int channels = audio_filter_context->channels <= 2 ? audio_filter_context->channels : 2;
    out_sample_channel_count[0] = channels;
    int64_t out_channel_layouts[] = {AV_CH_LAYOUT_MONO, -1};
    if (channels == 2) {
        out_channel_layouts[0] = AV_CH_LAYOUT_STEREO;
    }

    char args[512];
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channels=%d:channel_layout=0x%"PRIx64,
             time_base.num, time_base.den, dec_ctx->sample_rate,
             av_get_sample_fmt_name(dec_ctx->sample_fmt), audio_filter_context->channels,
             audio_filter_context->channel_layout);

    avfilter_graph_create_filter(&audio_filter_context->buffer_src_context, audio_filter_context->abuffer_src, "in", args, NULL,
                                 audio_filter_context->filter_graph);

    avfilter_graph_create_filter(&audio_filter_context->buffer_sink_context, audio_filter_context->abuffer_sink, "out", NULL, NULL,
                                 audio_filter_context->filter_graph);

    av_opt_set_int_list(audio_filter_context->buffer_sink_context, "sample_fmts", out_sample_fmts, -1,
                        AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int_list(audio_filter_context->buffer_sink_context, "channel_layouts", out_channel_layouts, -1,
                        AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int_list(audio_filter_context->buffer_sink_context, "sample_rates", out_sample_rates, -1,
                        AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int_list(audio_filter_context->buffer_sink_context, "channel_counts", out_sample_channel_count, -1,
                        AV_OPT_SEARCH_CHILDREN);

    audio_filter_context->outputs->name = av_strdup("in");
    audio_filter_context->outputs->filter_ctx = audio_filter_context->buffer_src_context;
    audio_filter_context->outputs->pad_idx = 0;
    audio_filter_context->outputs->next = NULL;

    audio_filter_context->inputs->name = av_strdup("out");
    audio_filter_context->inputs->filter_ctx = audio_filter_context->buffer_sink_context;
    audio_filter_context->inputs->pad_idx = 0;
    audio_filter_context->inputs->next = NULL;

    char filter_descr[128] = {0};
//    sprintf(filter_descr, "atempo=%.2f", speed);
    avfilter_graph_parse_ptr(audio_filter_context->filter_graph, "anull", &audio_filter_context->inputs, &audio_filter_context->outputs, NULL);

//    int ret = avfilter_link(audio_filter_context->buffer_src_context, 0, audio_filter_context->buffer_sink_context, 0);
//    if (ret < 0) {
//        LOGE("avfilter_link error: %s", av_err2str(ret));
//    }
    avfilter_graph_config(audio_filter_context->filter_graph, NULL);
//    audio_filter_context->outlink = audio_filter_context->buffer_sink_context->inputs[0];
    pthread_mutex_unlock(audio_filter_context->mutex);
//    avfilter_inout_free(&audio_filter_context->inputs);
//    avfilter_inout_free(&audio_filter_context->outputs);
}