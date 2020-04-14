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

#include "audio_encoder.h"
#include "android_xlog.h"

namespace trinity {

AudioEncoder::AudioEncoder()
    : codec_context_(nullptr),
      encode_frame_(nullptr),
      audio_next_pts_(0),
      audio_samples_data_(nullptr),
      audio_nb_frames_(0),
      audio_samples_size_(0),
      bit_rate_(0),
      channels_(0),
      sample_rate_(0),
      pcm_frame_callback_(nullptr),
      pcm_frame_context_(nullptr) {
}

AudioEncoder::~AudioEncoder() {}

int AudioEncoder::Init(int bit_rate, int channels, int sample_rate, const char *codec_name,
                       int (*PCMFrameCallback)(int16_t *, int, int, double *, void *context), void *context) {
    bit_rate_ = bit_rate;
    channels_ = channels;
    sample_rate_ = sample_rate;
    pcm_frame_callback_ = PCMFrameCallback;
    pcm_frame_context_ = context;
    AllocAudioStream(codec_name);
    AllocFrame();
    return 0;
}

int AudioEncoder::Encode(AudioPacketPool* audio_packet_pool) {
    double presentation_time_mills = -1;
    int sample_size = pcm_frame_callback_(reinterpret_cast<int16_t*>(audio_samples_data_[0]),
            audio_nb_frames_, channels_, &presentation_time_mills, pcm_frame_context_);
    if (sample_size <= 0) {
        LOGE("audio_frame_callback failed return size: %d", sample_size);
        return -1;
    }

    int frame_num = sample_size / channels_;
    int audio_sample_size = frame_num * channels_ * sizeof(short);
    AVRational time_base = { 1, sample_rate_ };
    encode_frame_->nb_samples = frame_num;
    avcodec_fill_audio_frame(encode_frame_, codec_context_->channels,
        codec_context_->sample_fmt, audio_samples_data_[0], audio_sample_size, 0);
    encode_frame_->pts = audio_next_pts_;
    audio_next_pts_ += encode_frame_->nb_samples;
    int ret = avcodec_send_frame(codec_context_, encode_frame_);
    if (ret < 0) {
        LOGE("audio send frame error: %s", av_err2str(ret));
        return ret;
    }
    while (ret >= 0) {
        AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        pkt.duration = AV_NOPTS_VALUE;
        pkt.pts = pkt.dts = 0;
        ret = avcodec_receive_packet(codec_context_, &pkt);
        if (ret >= 0) {
            pkt.pts = av_rescale_q(encode_frame_->pts, codec_context_->time_base, time_base);
            auto packet = new AudioPacket();
            packet->data = new uint8_t[pkt.size];
            memcpy(packet->data, pkt.data, static_cast<size_t>(pkt.size));
            packet->size = pkt.size;
            packet->position = static_cast<float>((pkt.pts * av_q2d(time_base) * 1000.0f));
            audio_packet_pool->PushAudioPacketToQueue(packet);
        }
        av_packet_unref(&pkt);
    }
    return 0;
}

void AudioEncoder::Destroy() {
    if (nullptr != audio_samples_data_[0]) {
        av_free(audio_samples_data_[0]);
        audio_samples_data_[0] = nullptr;
    }
    if (nullptr != encode_frame_) {
        av_free(encode_frame_);
        encode_frame_ = nullptr;
    }
    if (nullptr != codec_context_) {
        avcodec_close(codec_context_);
        av_free(codec_context_);
        codec_context_ = nullptr;
    }
}

int AudioEncoder::AllocFrame() {
    encode_frame_ = av_frame_alloc();
    if (nullptr == encode_frame_) {
        LOGE("alloc audio frame error");
        return -1;
    }
    encode_frame_->nb_samples = codec_context_->frame_size;
    encode_frame_->format = codec_context_->sample_fmt;
    encode_frame_->sample_rate = codec_context_->sample_rate;
    encode_frame_->channel_layout = codec_context_->channel_layout;

    /**
      * 这个地方原来是10000就导致了编码frame的时候有9个2048一个1568 其实编码解码是没有问题的
      * 但是在我们后续的sox处理的时候就会有问题，这里一定要注意注意 所以改成了10240
      */

    audio_nb_frames_ = (codec_context_->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
                        ? 10240 : codec_context_->frame_size;
    int src_sample_line_size;
    int ret = av_samples_alloc_array_and_samples(&audio_samples_data_, &src_sample_line_size,
            codec_context_->channels, audio_nb_frames_, codec_context_->sample_fmt, 0);
    if (ret < 0) {
        LOGE("alloc source samples error: %s", av_err2str(ret));
        return -2;
    }
    audio_samples_size_ = av_samples_get_buffer_size(nullptr,
        codec_context_->channels, audio_nb_frames_, codec_context_->sample_fmt, 0);
    return ret;
}

int AudioEncoder::AllocAudioStream(const char *codec_name) {
    AVCodec* codec = avcodec_find_encoder_by_name(codec_name);
    if (codec == nullptr) {
        LOGE("find audio encoder error: %s", codec_name);
        return -1;
    }
    codec_context_ = avcodec_alloc_context3(codec);
    codec_context_->codec_type = AVMEDIA_TYPE_AUDIO;
    codec_context_->sample_rate = sample_rate_;
    codec_context_->bit_rate = bit_rate_;
    codec_context_->sample_fmt = AV_SAMPLE_FMT_S16;
    codec_context_->channel_layout = channels_ == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
    codec_context_->channels = av_get_channel_layout_nb_channels(codec_context_->channel_layout);
    codec_context_->profile = FF_PROFILE_AAC_LOW;
    codec_context_->flags |= CODEC_FLAG_GLOBAL_HEADER;
    codec_context_->codec_id = codec->id;
    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        LOGE("open audio codec error");
        return -2;
    }
    return 0;
}

}  // namespace trinity
