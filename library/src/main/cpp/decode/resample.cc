/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//
// Created by wlanjie on 2019/4/20.
//

#include "resample.h"

namespace trinity {

Resample::Resample()
    : src_rate_(0),
      dst_rate_(0),
      src_nb_samples_(0),
      src_ch_layout_(0),
      dst_ch_layout_(0),
      src_nb_channels_(0),
      dst_nb_channels_(0),
      src_line_size_(0),
      dst_line_size_(0),
      dst_nb_samples_(0),
      max_dst_nb_samples_(0),
      dst_data_(nullptr),
      src_data_(nullptr),
      dst_sample_fmt_(AV_SAMPLE_FMT_NONE),
      dst_buffer_size_(0),
      swr_context_(nullptr) {
}

Resample::~Resample() {}

int Resample::Init(int src_rate, int dst_rate,
        int max_src_nb_samples, int src_channels,
        int dst_channels) {
    src_rate_ = src_rate;
    dst_rate_ = dst_rate;
    src_nb_samples_ = max_src_nb_samples;
    src_ch_layout_ = src_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
    dst_ch_layout_ = dst_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
    AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;
    dst_sample_fmt_ = AV_SAMPLE_FMT_S16;
    swr_context_ = swr_alloc();
    if (nullptr == swr_context_) {
        Destroy();
        return AVERROR(ENOMEM);
    }
    int ret;
    /* set options */
    av_opt_set_int(swr_context_, "in_channel_layout", src_ch_layout_, 0);
    av_opt_set_int(swr_context_, "in_sample_rate", src_rate, 0);
    av_opt_set_sample_fmt(swr_context_, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_context_, "out_channel_layout", dst_ch_layout_, 0);
    av_opt_set_int(swr_context_, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(swr_context_, "out_sample_fmt", dst_sample_fmt_, 0);

    if ((ret = swr_init(swr_context_)) < 0) {
        fprintf(stderr, "Failed to Initialize the resampling context_\n");
        Destroy();
        return ret;
    }

    src_nb_channels_ = av_get_channel_layout_nb_channels(src_ch_layout_);
    ret = av_samples_alloc_array_and_samples(&src_data_, &src_line_size_, src_nb_channels_, src_nb_samples_, src_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        Destroy();
        return ret;
    }

    max_dst_nb_samples_ = dst_nb_samples_ = av_rescale_rnd(src_nb_samples_, dst_rate, src_rate, AV_ROUND_UP);
    dst_nb_channels_ = av_get_channel_layout_nb_channels(dst_ch_layout_);
    ret = av_samples_alloc_array_and_samples(&dst_data_, &dst_line_size_, dst_nb_channels_, dst_nb_samples_, dst_sample_fmt_, 0);
    if (ret < 0 || dst_data_[0] == nullptr) {
        Destroy();
        return ret;
    }

    dst_nb_samples_ = av_rescale_rnd(swr_get_delay(swr_context_, src_rate) + src_nb_samples_, dst_rate, src_rate, AV_ROUND_UP);

    if (dst_nb_samples_ > max_dst_nb_samples_) {
        av_free(dst_data_[0]);
        ret = av_samples_alloc(dst_data_, &dst_line_size_, dst_nb_channels_, dst_nb_samples_, dst_sample_fmt_, 1);
        if (ret < 0) {
            Destroy();
        }
        max_dst_nb_samples_ = dst_nb_samples_;
    }
    dst_buffer_size_ = av_samples_get_buffer_size(&dst_line_size_, dst_nb_channels_, dst_nb_samples_, dst_sample_fmt_, 1);
    return ret;
}

int Resample::Process(short *in, uint8_t *out, int src_nb_samples, int *out_nb_samples) {
    memcpy(src_data_[0], in, src_nb_samples * 2); // TODO: change to src_data[0] = in_data
    return Convert(out, src_nb_samples, out_nb_samples);
}

int Resample::Process(short **in, uint8_t *out, int src_nb_samples, int *out_nb_samples) {
    short *dstp = (short *) src_data_[0];
    for (int i = 0; i < src_nb_samples; i++) {
        dstp[2 * i] = in[0][i];
        dstp[2 * i + 1] = in[1][i];
    }
    return Convert(out, src_nb_samples, out_nb_samples);
}

int Resample::Convert(uint8_t *out, int src_nb_samples, int *out_nb_samples) {
    int real_dst_nb_samples = dst_nb_samples_;
    int real_dst_bufsize = dst_buffer_size_;
    if (src_nb_samples != src_nb_samples_) {
        real_dst_nb_samples = av_rescale_rnd(src_nb_samples, dst_rate_, src_rate_, AV_ROUND_UP);
        int real_dst_linesize = 0;
        real_dst_bufsize = av_samples_get_buffer_size(&real_dst_linesize, dst_nb_channels_, real_dst_nb_samples,
                                                      dst_sample_fmt_, 1);
    }
    int ret = swr_convert(swr_context_, dst_data_, real_dst_nb_samples, (const uint8_t **) src_data_, src_nb_samples);
    if (ret < 0) {
        return ret;
    }
    if (dst_data_[0] != nullptr || out == nullptr) {
        memcpy(out, dst_data_[0], real_dst_bufsize);
    }
    if (out_nb_samples) {
        *out_nb_samples = real_dst_bufsize;
    }
    return ret;
}

void Resample::Destroy() {
    if (nullptr != src_data_) {
        av_freep(&src_data_[0]);
        av_freep(&src_data_);
        src_data_ = nullptr;
    }
    if (nullptr != dst_data_) {
        av_freep(&dst_data_[0]);
        av_freep(&dst_data_);
        dst_data_ = nullptr;
    }
    if (nullptr != swr_context_) {
        swr_free(&swr_context_);
        swr_context_ = nullptr;
    }
}

}  // namespace trinity
