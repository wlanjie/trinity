#include "resampler.h"

#define LOG_TAG "Resampler"

Resampler::Resampler() {

}

Resampler::~Resampler() {

}

/*return <0 on failure*/
int Resampler::init(int _src_rate, int _dst_rate, int _max_src_nb_samples, int src_channels, int to_channels) {
    src_rate = _src_rate;
    dst_rate = _dst_rate;
    src_nb_samples = _max_src_nb_samples;
    LOGI("src_channels is %d to_channels is %d", src_channels, to_channels);
    src_ch_layout = (src_channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO);
    dst_ch_layout = (to_channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO);
    AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;
    dst_sample_fmt = AV_SAMPLE_FMT_S16;
    int ret = 0;
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler_ context_\n");
        ret = AVERROR(ENOMEM);
        destroy();
        return ret;
    }

    /* set options */
    av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to Initialize the resampling context_\n");
        destroy();
        return ret;
    }

    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels, src_nb_samples, src_sample_fmt,
                                             0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        destroy();
        return ret;
    }

    max_dst_nb_samples = dst_nb_samples = av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt,
                                             0);
    if (ret < 0 || dst_data[0] == NULL) {
        destroy();
        return ret;
    }

    dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) + src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    if (dst_nb_samples > max_dst_nb_samples) {
        av_free(dst_data[0]);
        ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 1);
        if (ret < 0)
            destroy();
        max_dst_nb_samples = dst_nb_samples;
    }
    dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 1);
    return ret;
}

int Resampler::process(short *in_data, uint8_t *out_data, int real_src_nb_samples, int *out_nb_bytes) {
    memcpy(src_data[0], in_data, real_src_nb_samples * 2);//TODO: change to src_data[0] = in_data
    return convert(out_data, real_src_nb_samples, out_nb_bytes);
}

int Resampler::convert(uint8_t *out_data, int real_src_nb_samples, int *out_nb_bytes) {
    int real_dst_nb_samples = dst_nb_samples;
    int real_dst_bufsize = dst_bufsize;
    if (real_src_nb_samples != src_nb_samples) {
        real_dst_nb_samples = av_rescale_rnd(real_src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        int real_dst_linesize = 0;
        real_dst_bufsize = av_samples_get_buffer_size(&real_dst_linesize, dst_nb_channels, real_dst_nb_samples,
                                                      dst_sample_fmt, 1);
    }
    int ret = swr_convert(swr_ctx, dst_data, real_dst_nb_samples, (const uint8_t **) src_data, real_src_nb_samples);
    if (ret < 0) {
        return ret;
    }
    if (dst_data[0] != NULL || out_data == NULL) {
        memcpy(out_data, dst_data[0], real_dst_bufsize);
    }
    if (out_nb_bytes) {
        *out_nb_bytes = real_dst_bufsize;
    }
    return ret;
}

int Resampler::process(short **in_data, uint8_t *out_data, int real_src_nb_samples, int *out_nb_bytes) {
    short *dstp = (short *) src_data[0];
    for (int i = 0; i < real_src_nb_samples; i++) {
        dstp[2 * i] = in_data[0][i];
        dstp[2 * i + 1] = in_data[1][i];
    }
    return convert(out_data, real_src_nb_samples, out_nb_bytes);
}

void Resampler::destroy() {
    if (src_data)
        av_freep(&src_data[0]);
    av_freep(&src_data);
    if (dst_data)
        av_freep(&dst_data[0]);
    av_freep(&dst_data);
    swr_free(&swr_ctx);
}

