//
// Created by wlanjie on 2019-06-15.
//

#include "error_code.h"
#include "video_export.h"
#include "soft_encoder_adapter.h"
#include "android_xlog.h"
#include "tools.h"
#include <math.h>

namespace trinity {

VideoExport::VideoExport() {
    export_ing = false;
    egl_core_ = nullptr;
    egl_surface_ = EGL_NO_SURFACE;
    video_state_ = nullptr;
    yuv_render_ = nullptr;
    export_index_ = 0;
    video_width_ = 0;
    video_height_ = 0;
    frame_width_ = 0;
    frame_height_ = 0;
    encoder_ = nullptr;
    packet_thread_ = nullptr;
}

VideoExport::~VideoExport() {

}

int VideoExport::FrameQueueInit(FrameQueue *f, PacketQueue *pktq, int maxSize, int keepLast) {
    memset(f, 0, sizeof(FrameQueue));
    int result = pthread_mutex_init(&f->mutex, nullptr);
    if (result != 0) {
        av_log(nullptr, AV_LOG_FATAL, "Frame Init mutex error");
        return AVERROR(ENOMEM);
    }
    result = pthread_cond_init(&f->cond, nullptr);
    if (result != 0) {
        av_log(nullptr, AV_LOG_FATAL, "Frame Init cond error");
        return AVERROR(ENOMEM);
    }
    f->packet_queue = pktq;
    f->max_size = maxSize;
    f->keep_last = keepLast;
    for (int i = 0; i < f->max_size; i++) {
        if (!(f->queue[i].frame = av_frame_alloc())) {
            return AVERROR(ENOMEM);
        }
    }
    return 0;
}

Frame* VideoExport::FrameQueuePeekWritable(FrameQueue* f) {
    pthread_mutex_lock(&f->mutex);
    while (f->size >= f->max_size && !f->packet_queue->abort_request) {
        pthread_cond_wait(&f->cond, &f->mutex);
    }
    pthread_mutex_unlock(&f->mutex);
    if (f->packet_queue->abort_request) {
        return nullptr;
    }
    return &f->queue[f->windex];
}

Frame* VideoExport::FrameQueuePeekReadable(FrameQueue* f) {
    pthread_mutex_lock(&f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->packet_queue->abort_request) {
        pthread_cond_wait(&f->cond, &f->mutex);
    }
    pthread_mutex_unlock(&f->mutex);
    if (f->packet_queue->abort_request) {
        return nullptr;
    }
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void VideoExport::FrameQueuePush(FrameQueue* f) {
    if (++f->windex == f->max_size) {
        f->windex = 0;
    }
    pthread_mutex_lock(&f->mutex);
    f->size++;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

Frame* VideoExport::FrameQueuePeek(FrameQueue* f) {
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

int VideoExport::FrameQueueNbRemaining(FrameQueue* f) {
    return f->size - f->rindex_shown;
}

Frame* VideoExport::FrameQueuePeekLast(FrameQueue* f) {
    return &f->queue[f->rindex];
}

Frame* VideoExport::FrameQueuePeekNext(FrameQueue* f) {
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

void VideoExport::FrameQueueUnrefItem(Frame* vp) {
    av_frame_unref(vp->frame);
}

void VideoExport::FrameQueueNext(FrameQueue* f) {
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    FrameQueueUnrefItem(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size) {
        f->rindex = 0;
    }
    pthread_mutex_lock(&f->mutex);
    f->size--;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

void VideoExport::FrameQueueSignal(FrameQueue* f) {
    pthread_mutex_lock(&f->mutex);
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

void VideoExport::FrameQueueDestroy(FrameQueue* f) {
    for (int i = 0; i < f->max_size; i++) {
        Frame* vp = &f->queue[i];
        FrameQueueUnrefItem(vp);
        av_frame_free(&vp->frame);
    }
    pthread_mutex_destroy(&f->mutex);
    pthread_cond_destroy(&f->cond);
}

int VideoExport::PacketQueueInit(PacketQueue* q) {
    memset(q, 0, sizeof(PacketQueue));
    int result = pthread_mutex_init(&q->mutex, nullptr);
    if (result != 0) {
        av_log(nullptr, AV_LOG_FATAL, "Packet Init create mutex error.");
        return AVERROR(ENOMEM);
    }
    result = pthread_cond_init(&q->cond, nullptr);
    if (result != 0) {
        av_log(nullptr, AV_LOG_FATAL, "Packet Init create cond error.");
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

int VideoExport::PacketQueueGet(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    MyAVPacketList* pkt1;
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
                q->last_pkt = nullptr;
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

int VideoExport::PacketQueuePut(PacketQueue *q, AVPacket *pkt) {
    int ret;
    pthread_mutex_lock(&q->mutex);
    ret = PacketQueuePutPrivate(q, pkt);
    pthread_mutex_unlock(&q->mutex);
    if (pkt != &flush_packet_ && ret < 0) {
        av_packet_unref(pkt);
    }
    return ret;
}

int VideoExport::PacketQueuePutPrivate(PacketQueue* q, AVPacket* packet) {
    if (q->abort_request) {
        return -1;
    }
    MyAVPacketList* pkt1 = reinterpret_cast<MyAVPacketList*>(av_malloc(sizeof(MyAVPacketList)));
    if (pkt1 == nullptr) {
        return AVERROR(ENOMEM);
    }
    pkt1->pkt = *packet;
    pkt1->next = nullptr;
    if (packet == &flush_packet_) {
        q->serial++;
    }
    pkt1->serial = q->serial;
    if (!q->last_pkt) {
        q->first_pkt = pkt1;
    } else{
        q->last_pkt->next = pkt1;
    }
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    pthread_cond_signal(&q->cond);
    return 0;
}

int VideoExport::PacketQueuePutNullPacket(PacketQueue *q, int streamIndex) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->stream_index = streamIndex;
    return PacketQueuePut(q, pkt);
}

void VideoExport::PacketQueueStart(PacketQueue* q) {
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 0;
    PacketQueuePutPrivate(q, &flush_packet_);
    pthread_mutex_unlock(&q->mutex);
}

void VideoExport::PacketQueueAbort(PacketQueue* q) {
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 1;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

void VideoExport::PacketQueueFlush(PacketQueue* q) {
    MyAVPacketList *pkt, *pkt1;
    pthread_mutex_lock(&q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = nullptr;
    q->first_pkt = nullptr;
    q->nb_packets = 0;
    q->size = 0;
    pthread_mutex_unlock(&q->mutex);
}

void VideoExport::PacketQueueDestroy(PacketQueue* q) {
    PacketQueueFlush(q);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void VideoExport::DecoderInit(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, pthread_cond_t empty_queue_cond) {
    memset(d, 0, sizeof(Decoder));
    d->codec_context = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
}

int VideoExport::DecoderStart(Decoder *d, void* (*fn) (void*), void *arg) {
    PacketQueueStart(d->queue);
    int result = pthread_create(&d->decoder_tid, NULL, fn, arg);
    if (result != 0) {
        av_log(NULL, AV_LOG_ERROR, "create decode thread failed: %d\n", result);
        return AVERROR(ENOMEM);
    }
    return 0;
}

int ConfigureFilterGraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx, AVFilterContext *sink_ctx) {
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

int ConfigureAudioFilters(VideoState *is, const char *afilters, int force_outout_format) {
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
        snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%" PRIx64, is->audio_filter_src.channel_layout);
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
    if ((ret = ConfigureFilterGraph(is->agraph, afilters, filt_asrc, filt_asink)) < 0) {
        LOGE("configure_filtergraph error: %d args: %s", ret, asrc_args);
        avfilter_graph_free(&is->agraph);
        return ret;
    }
    is->in_audio_filter = filt_asrc;
    is->out_audio_filter = filt_asink;
    return ret;
}

int VideoExport::StreamCommonOpen(int stream_index) {
    VideoState* is = video_state_;
    int ret;
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    AVDictionary *opts = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int stream_lowres = 0;
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
            if ((ret = ConfigureAudioFilters(is, NULL, 0)) < 0) {
                avcodec_free_context(&avctx);
                LOGE("configure_audio_filters failed: %d", ret);
                return ret;
            }
            link = is->out_audio_filter->inputs[0];
            sample_rate = link->sample_rate;
            channel_layout = link->channel_layout;
            nb_channels = link->channels;
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
            is->audio_diff_avg_coef = exp(log(0.01) / 20);
            is->audio_diff_avg_count = 0;

            /* since we do not have a precise anough audio fifo fullness,
             we correct audio sync only if larger than this threshold */
            is->audio_diff_threshold = (double) (is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;
            is->audio_stream = stream_index;
            is->audio_st = ic->streams[stream_index];

            DecoderInit(&is->audio_decode, avctx, &is->audioq, is->continue_read_thread);
            if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
                is->audio_decode.start_pts = is->audio_st->start_time;
                is->audio_decode.start_pts_tb = is->audio_st->time_base;
            }
            if ((ret = DecoderStart(&is->audio_decode, AudioThread, this)) < 0) {
                avcodec_free_context(&avctx);
                return ret;
            }
        }
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_stream = stream_index;
            is->video_st = ic->streams[stream_index];
            is->viddec_width = avctx->width;
            is->viddec_height = avctx->height;
            DecoderInit(&is->video_decode, avctx, &is->videoq, is->continue_read_thread);
            if ((ret = DecoderStart(&is->video_decode, VideoThread, this)) < 0) {
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

int VideoExport::ReadFrame() {
    VideoState* is = video_state_;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    memset(st_index, -1, sizeof(st_index));
    is->eof = 0;
    AVFormatContext* ic = avformat_alloc_context();
    if (!ic) {
        return -1;
    }
    int ret = avformat_open_input(&ic, is->filename, nullptr, nullptr);
    if (ret < 0) {
        return -1;
    }
    is->ic = ic;
    if (is->start_time != 0) {
        int64_t timestamp = is->start_time;

        if (ic->start_time != AV_NOPTS_VALUE) {
            timestamp += ic->start_time;
        }
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp * (AV_TIME_BASE / 1000), INT64_MAX, 0);
        if (ret < 0) {
            av_err2str(ret);
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                   is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }
    av_dump_format(ic, 0, is->filename, 0);
    char* wanted_stream_spec[] = { 0 };
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
            av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", wanted_stream_spec[i], av_get_media_type_string((AVMediaType) i));
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
        ret = StreamCommonOpen(st_index[AVMEDIA_TYPE_AUDIO]);
    }
    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = StreamCommonOpen(st_index[AVMEDIA_TYPE_VIDEO]);
    }

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        StreamCommonOpen(st_index[AVMEDIA_TYPE_SUBTITLE]);
    }
    if (is->video_stream < 0 && is->audio_stream < 0) {
        return NULL;
    }
    while (!is->abort_request) {
        if ((is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE ||
             ((is->audioq.nb_packets > MIN_FRAMES || is->audio_stream < 0 || is->audioq.abort_request) &&
              (is->videoq.nb_packets > MIN_FRAMES || is->video_stream < 0 || is->videoq.abort_request ||
               (is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) &&
              (is->subtitleq.nb_packets > MIN_FRAMES || is->subtitle_stream < 0 || is->subtitleq.abort_request)))) {
            // TODO sleep
            av_usleep(1000000);
            continue;
        }

        if (!is->paused &&
            (!is->audio_st || (is->audio_decode.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sample_queue) == 0)) &&
            (!is->video_st || (is->video_decode.finished == is->videoq.serial && frame_queue_nb_remaining(&is->video_queue) == 0))) {
            is->paused = 0;
            is->audio_decode.finished = 0;
            is->video_decode.finished = 0;
            // TODO next file
            export_ing = false;
        }
        // 播放到指定时间
        // 如果是循环播放 seek到开始时间
        if (is->finish) {
            is->finish = 0;
            // TODO next file
            export_ing = false;
        }

        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0) {
                    PacketQueuePutNullPacket(&is->videoq, is->video_stream);
                }
                if (is->audio_stream >= 0) {
                    PacketQueuePutNullPacket(&is->audioq, is->audio_stream);
                }
                if (is->subtitle_stream >= 0) {
                    PacketQueuePutNullPacket(&is->subtitleq, is->subtitle_stream);
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
        int64_t stream_start_time = ic->streams[pkt->stream_index]->start_time;
        int64_t pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        if (pkt->stream_index == is->audio_stream) {
            PacketQueuePut(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            PacketQueuePut(&is->videoq, pkt);
        } else if (pkt->stream_index == is->subtitle_stream) {
            PacketQueuePut(&is->subtitleq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }
    return 0;
}

int VideoExport::DecodeFrame(VideoState* is, Decoder *d, AVFrame *frame, AVSubtitle *sub) {
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
                if (PacketQueueGet(d->queue, &pkt, 1, &d->pkt_serial) < 0) {
                    return -1;
                }
                if (pkt.data == flush_packet_.data) {
                    avcodec_flush_buffers(d->codec_context);
                    d->finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                }
            } while (pkt.data == flush_packet_.data || d->queue->serial != d->pkt_serial);
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

int64_t GetValidChannelLayout(int64_t channel_layout, int channels) {
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels) {
        return channel_layout;
    } else {
        return 0;
    }
}

int CmpAudioFormats(enum AVSampleFormat fmt1, int64_t channel_count1, enum AVSampleFormat fmt2, int64_t channel_count2) {
    if (channel_count1 == 1 && channel_count2 == 1) {
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    } else {
        return channel_count1 != channel_count2 || fmt1 != fmt2;
    }
}

int VideoExport::DecoderAudio() {
    VideoState *is = video_state_;
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
        if ((got_frame = DecodeFrame(is, &is->audio_decode, frame, NULL)) < 0) {
            LOGE("decoder_decode_frame: %d", got_frame);
            avfilter_graph_free(&is->agraph);
            av_frame_free(&frame);
            return NULL;
        }
        if (got_frame) {
            tb = (AVRational) { 1, frame->sample_rate };
            dec_channel_layout = GetValidChannelLayout(frame->channel_layout, av_frame_get_channels(frame));
            reconfigure = CmpAudioFormats(is->audio_filter_src.fmt, is->audio_filter_src.channels, (AVSampleFormat) frame->format, av_frame_get_channels(frame));
            if (reconfigure) {
                char buf1[1024], buf2[1024];
                av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
                av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
                av_log(NULL, AV_LOG_DEBUG,
                       "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
                       is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
                       frame->sample_rate, av_frame_get_channels(frame), av_get_sample_fmt_name((AVSampleFormat) frame->format), buf2, is->audio_decode.pkt_serial);
                is->audio_filter_src.fmt = (AVSampleFormat) frame->format;
                is->audio_filter_src.channel_layout = dec_channel_layout;
                is->audio_filter_src.channels = av_frame_get_channels(frame);
                is->audio_filter_src.freq = frame->sample_rate;
                last_serial = is->audio_decode.pkt_serial;
                if ((ret = ConfigureAudioFilters(is, NULL, 1)) < 0) {
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
                FrameQueuePush(&is->sample_queue);
                if (is->audioq.serial != is->audio_decode.pkt_serial) {
                    break;
                }
            }
            if (ret == AVERROR_EOF) {
                is->audio_decode.finished = is->audio_decode.pkt_serial;
            }
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    return 0;
}

int ConfigureVideoFilters(AVFilterGraph *graph, VideoState *is, const char *vfilters, AVFrame *frame) {
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
    if ((ret = ConfigureFilterGraph(graph, vfilters, filt_src, last_filter)) < 0) {
        return ret;
    }
    is->in_video_filter = filt_src;
    is->out_video_filter = filt_out;
    return ret;
}

int VideoExport::GetVideoFrame(VideoState *is, AVFrame *frame) {
    int got_picture;
    if ((got_picture = DecodeFrame(is, &is->video_decode, frame, NULL)) < 0) {
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

int VideoExport::QueuePicture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial) {
    Frame *vp;
    if (!(vp = FrameQueuePeekWritable(&is->video_queue))) {
        return -1;
    }
    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;
    av_frame_move_ref(vp->frame, src_frame);
    FrameQueuePush(&is->video_queue);
    return 0;
}

int VideoExport::DecoderVideo() {
    int ret;
    VideoState *is = video_state_;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);
    AVFilterGraph *graph = avfilter_graph_alloc();
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = AV_PIX_FMT_NONE;
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
        ret = GetVideoFrame(is, frame);
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
            if ((ret = ConfigureVideoFilters(graph, is, NULL, frame)) < 0) {
                avfilter_graph_free(&graph);
                av_frame_free(&frame);
                return NULL;
            }
            filt_in = is->in_video_filter;
            filt_out = is->out_video_filter;
            last_w = frame->width;
            last_h = frame->height;
            last_format = (AVPixelFormat) frame->format;
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
            ret = QueuePicture(is, frame, pts, duration, av_frame_get_pkt_pos(frame), is->video_decode.pkt_serial);
            av_frame_unref(frame);
        }
    }
    return 0;
}

void* VideoExport::ReadThread(void *arg) {
    VideoExport* video_export = reinterpret_cast<VideoExport*>(arg);
    video_export->ReadFrame();
    pthread_exit(0);
}

void* VideoExport::AudioThread(void *arg) {
    VideoExport* video_export = reinterpret_cast<VideoExport*>(arg);
    video_export->DecoderAudio();
    pthread_exit(0);
}

void* VideoExport::VideoThread(void *arg) {
    VideoExport* video_export = reinterpret_cast<VideoExport*>(arg);
    video_export->DecoderVideo();
    pthread_exit(0);
}

int VideoExport::StreamOpen(VideoState* is, char *file_name) {
    av_init_packet(&flush_packet_);
    flush_packet_.data = (uint8_t*) &flush_packet_;
    video_state_->filename = av_strdup(file_name);
    if (FrameQueueInit(&is->video_queue, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0) {
        StreamClose(is);
        return -1;
    }
    if (FrameQueueInit(&is->subtitle_queue, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0) {
        StreamClose(is);
        return -1;
    }
    if (FrameQueueInit(&is->sample_queue, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0) {
        StreamClose(is);
        return -1;
    }
    if (PacketQueueInit(&is->videoq) < 0 || PacketQueueInit(&is->audioq) < 0 || PacketQueueInit(&is->subtitleq) < 0) {
        StreamClose(is);
        return -1;
    }
    int result = pthread_create(&is->read_tid, nullptr, ReadThread, this);
    if (result != 0) {
        StreamClose(is);
        return -1;
    }
    return 0;
}

void VideoExport::StreamClose(VideoState* is) {

}

int VideoExport::Export(deque<MediaClip*> clips, const char *path, int width, int height, int frame_rate, int video_bit_rate,
        int sample_rate, int channel_count, int audio_bit_rate) {
    if (clips.empty()) {
        return CLIP_EMPTY;
    }
    export_ing = true;
    packet_thread_ = new VideoConsumerThread();
    int ret = packet_thread_->Init(path, width, height, frame_rate, video_bit_rate, 0, channel_count, audio_bit_rate, "libfdk_aac");
    if (ret < 0) {
        return ret;
    }
    PacketPool::GetInstance()->InitRecordingVideoPacketQueue();
    PacketPool::GetInstance()->InitAudioPacketQueue(44100);
    AudioPacketPool::GetInstance()->InitAudioPacketQueue();
    packet_thread_->StartAsync();

    video_width_ = width;
    video_height_ = height;
    // copy clips to export clips
    for (int i = 0; i < clips.size(); ++i) {
        MediaClip* clip = clips.at(i);
        MediaClip* export_clip = new MediaClip();
        export_clip->start_time = clip->start_time;
        export_clip->end_time  = clip->end_time;
        export_clip->file_name = clip->file_name;
        clip_deque_.push_back(export_clip);
    }
    encoder_ = new SoftEncoderAdapter(0);
    encoder_->Init(width, height, video_bit_rate, frame_rate);
    MediaClip* clip = clip_deque_.at(0);
    video_state_ = static_cast<VideoState*>(av_malloc(sizeof(VideoState)));

    video_state_->start_time = 0;
    video_state_->end_time = INT64_MAX;
    StreamOpen(video_state_, clip->file_name);
    pthread_create(&export_thread_, nullptr, ExportThread, this);
    return 0;
}

void* VideoExport::ExportThread(void* context) {
    VideoExport* video_export = (VideoExport*) (context);
    video_export->ProcessExport();
    pthread_exit(0);
}

void VideoExport::ProcessExport() {
    egl_core_ = new EGLCore();
    egl_core_->InitWithSharedContext();
    egl_surface_ = egl_core_->CreateOffscreenSurface(64, 64);
    if (nullptr == egl_surface_ || EGL_NO_SURFACE == egl_surface_) {
        return;
    }
    egl_core_->MakeCurrent(egl_surface_);
    FrameBuffer* frame_buffer = new FrameBuffer(video_width_, video_height_, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    image_process_ = new ImageProcess();
    encoder_->CreateEncoder(egl_core_, frame_buffer->GetTextureId());
    while (export_ing) {
        if (video_state_->video_queue.size == 0) {
            av_usleep(10000);
            continue;
        }
        if (FrameQueueNbRemaining(&video_state_->video_queue) == 0) {
            continue;
        }
        LOGE("Peek size: %d", video_state_->video_queue.size);
        Frame* vp = FrameQueuePeek(&video_state_->video_queue);
        if (vp->serial != video_state_->videoq.serial) {
            FrameQueueNext(&video_state_->video_queue);
            continue;
        }
        FrameQueueNext(&video_state_->video_queue);
        Frame* frame = FrameQueuePeekLast(&video_state_->video_queue);
        if (frame->frame != nullptr) {
            if (isnan(frame->pts)) {
                break;
            }
            if (!frame->uploaded && frame->frame != nullptr) {
                int width = MIN(frame->frame->linesize[0], frame->frame->width);
                int height = frame->frame->height;
                if (frame_width_ != width || frame_height_ != height) {
                    frame_width_ = width;
                    frame_height_ = height;
                    if (nullptr != yuv_render_) {
                        delete yuv_render_;
                    }
                    yuv_render_ = new YuvRender(frame->width, frame->height, 0);
                }
                int texture_id = yuv_render_->DrawFrame(frame->frame);
                uint64_t current_time = (uint64_t) (frame->frame->pts * av_q2d(video_state_->video_st->time_base) * 1000);
                texture_id = image_process_->Process(texture_id, current_time, frame->width, frame->height, 0, 0);
                frame_buffer->OnDrawFrame(texture_id);
                if (!egl_core_->SwapBuffers(egl_surface_)) {
                    LOGE("eglSwapBuffers error: %d", eglGetError());
                }
                encoder_->Encode(current_time);
                frame->uploaded = 1;
            }
        }
    }
    encoder_->DestroyEncoder();
    delete encoder_;
    LOGE("export done");
    packet_thread_->Stop();
    PacketPool::GetInstance()->DestroyRecordingVideoPacketQueue();
    PacketPool::GetInstance()->DestroyAudioPacketQueue();
    AudioPacketPool::GetInstance()->DestroyAudioPacketQueue();
    delete packet_thread_;

    egl_core_->ReleaseSurface(egl_surface_);
    egl_core_->Release();
    egl_surface_ = EGL_NO_SURFACE;
    delete egl_core_;
}

}