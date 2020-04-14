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
 *
 */

//
// Created by wlanjie on 2019/4/18.
//

#include "mp4_muxer.h"
#include "android_xlog.h"
#include "tools.h"

#define is_start_code(code)	(((code) & 0x0ffffff) == 0x01)

namespace trinity {

Mp4Muxer::Mp4Muxer()
    : header_data_(nullptr),
      header_size_(0),
      format_context_(nullptr),
      video_stream_(nullptr),
      audio_stream_(nullptr),
      bit_stream_filter_context_(nullptr),
//      bit_stream_filter_(nullptr),
      duration_(0),
      last_audio_packet_presentation_time_mills_(0),
      last_video_presentation_time_ms_(0),
      video_width_(0),
      video_height_(0),
      video_frame_rate_(0),
      video_bit_rate_(0),
      audio_sample_rate_(0),
      audio_channels_(0),
      audio_bit_rate_(0),
      audio_packet_callback_(nullptr),
      audio_packet_context_(nullptr),
      video_packet_callback_(nullptr),
      video_packet_context_(nullptr),
      write_header_success_(false) {
}

Mp4Muxer::~Mp4Muxer() {}

int Mp4Muxer::Init(const char *path, int video_width, int video_height, int frame_rate, int video_bit_rate,
                            int audio_sample_rate, int audio_channels, int audio_bit_rate, char *audio_codec_name) {
    duration_ = 0;
    format_context_ = nullptr;
    video_stream_ = nullptr;
    audio_stream_ = nullptr;
    video_width_ = video_width;
    video_height_ = video_height;
    video_frame_rate_ = frame_rate;
    video_bit_rate_ = video_bit_rate;
    audio_sample_rate_ = audio_sample_rate;
    audio_channels_ = audio_channels;
    audio_bit_rate_ = audio_bit_rate;
    int ret = avformat_alloc_output_context2(&format_context_, nullptr, "mp4", path);
    if (ret != 0) {
        LOGE("alloc output context error: %d", ret);
        return ret;
    }
    if (format_context_ == nullptr) {
        return 0;
    }
    ret = BuildVideoStream();
    if (ret != 0) {
        LOGE("add video stream error: %d", ret);
        return ret;
    }
    if (audio_sample_rate > 0 && audio_channels > 0 && audio_bit_rate > 0) {
        ret = BuildAudioStream(audio_codec_name);
        if (ret != 0) {
            LOGE("add audio stream error: %d", ret);
            return ret;
        }
    }
    if (!(format_context_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&format_context_->pb, path, AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret != 0) {
            LOGE("avio open error: %d message: %s", ret, av_err2str(ret));
            return ret;
        }
    }
    AVCodec* codec = av_codec_next(nullptr);
    while (codec != nullptr) {
        switch (codec->type) {
            case AVMEDIA_TYPE_VIDEO:
                LOGI("[Video]:%s", codec->name);
                break;
            case AVMEDIA_TYPE_AUDIO:
                LOGI("[Audio]:%s", codec->name);
                break;
            default:
                LOGI("[Other]:%s", codec->name);
                break;
        }
        codec = codec->next;
    }
    return 0;
}

void Mp4Muxer::RegisterAudioPacketCallback(int (*audio_packet)(AudioPacket **, void *context, bool wait), void *context) {
    audio_packet_callback_ = audio_packet;
    audio_packet_context_ = context;
}

void Mp4Muxer::RegisterVideoPacketCallback(int (*video_packet)(VideoPacket **, void *context, bool wait), void *context) {
    video_packet_callback_ = video_packet;
    video_packet_context_ = context;
}

int Mp4Muxer::Encode() {
    double video_time = GetVideoStreamTimeInSecs();
    double audio_time = GetAudioStreamTimeInSecs();
    int ret = 0;
    /* write interleaved audio and video frames */
    if (!video_stream_ || (video_stream_ && audio_stream_ && audio_time < video_time)) {
        ret = WriteAudioFrame(format_context_, audio_stream_, false);
    } else if (video_stream_) {
        ret = WriteVideoFrame(format_context_, video_stream_);
    }
    duration_ = MIN(audio_time, video_time);
    return ret;
}

int Mp4Muxer::Stop() {
    LOGE("enter: %s", __func__);
    // flush audio
    if (nullptr != audio_stream_) {
        int ret = WriteAudioFrame(format_context_, audio_stream_, false);
        while (ret >= 0) {
            ret = WriteAudioFrame(format_context_, audio_stream_, false);
        }
    }
    // flush video
    if (nullptr != video_stream_) {
        int ret = WriteVideoFrame(format_context_, video_stream_, false);
        while (ret >= 0) {
            ret = WriteVideoFrame(format_context_, video_stream_, false);
        }
    }
    if (write_header_success_) {
        av_write_trailer(format_context_);
    }
    if (nullptr != video_stream_) {
        CloseVideo(format_context_, video_stream_);
        video_stream_ = nullptr;
    }
    if (nullptr != audio_stream_) {
        CloseAudio(format_context_, audio_stream_);
        audio_stream_ = nullptr;
    }
    if (nullptr != format_context_) {
        if (!(format_context_->oformat->flags & AVFMT_NOFILE)) {
            avio_close(format_context_->pb);
        }

        avformat_free_context(format_context_);
        format_context_ = nullptr;
    }
    if (nullptr != header_data_) {
        delete[] header_data_;
        header_data_ = nullptr;
    }
    LOGE("leave: %s", __func__);
    return 0;
}

AVStream* Mp4Muxer::AddStream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, char *codec_name) {
    if (AV_CODEC_ID_NONE == codec_id) {
        *codec = avcodec_find_encoder_by_name(codec_name);
    } else {
        *codec = avcodec_find_encoder(codec_id);
    }
    if (!(*codec)) {
        LOGE("find encoder error: %s", avcodec_get_name(codec_id));
        return nullptr;
    }
    AVStream* stream = avformat_new_stream(oc, *codec);
    if (stream == nullptr) {
        LOGE("alloc stream error");
        return nullptr;
    }
    stream->id = oc->nb_streams - 1;
    AVCodecContext* context = stream->codec;
    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
            context->sample_fmt = AV_SAMPLE_FMT_S16;
            context->bit_rate = audio_bit_rate_;
            context->codec_type = AVMEDIA_TYPE_AUDIO;
            context->sample_rate = audio_sample_rate_;
            context->channel_layout = audio_channels_ == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
            context->channels = av_get_channel_layout_nb_channels(context->channel_layout);
            context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        case AVMEDIA_TYPE_VIDEO:
            context->codec_id = AV_CODEC_ID_H264;
            context->bit_rate = video_bit_rate_;
            context->width = video_width_;
            context->height = video_height_;
            AVRational video_time_base = { 1, 10000 };
            stream->avg_frame_rate = video_time_base;
            context->time_base = video_time_base;
            stream->time_base = video_time_base;
            context->gop_size = static_cast<int>(video_frame_rate_);
            context->qmin = 10;
            context->qmax = 30;
            context->pix_fmt = AV_PIX_FMT_YUV420P;
            // 新增语句，设置为编码延迟
            av_opt_set(context->priv_data, "preset", "ultrafast", 0);
            // 实时编码关键看这句，上面那条无所谓
            av_opt_set(context->priv_data, "tune", "zerolatency", 0);
            if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
                context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            break;
    }
    if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    return stream;
}

int Mp4Muxer::WriteAudioFrame(AVFormatContext *oc, AVStream *st, bool wait) {
    AudioPacket* audio_packet = nullptr;
    int ret = audio_packet_callback_(&audio_packet, audio_packet_context_, wait);
    if (ret < 0 || audio_packet == nullptr) {
        return ret;
    }
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    last_audio_packet_presentation_time_mills_ = audio_packet->position;
    pkt.data = audio_packet->data;
    pkt.size = audio_packet->size;
    pkt.dts = pkt.pts = (int64_t) (last_audio_packet_presentation_time_mills_ / 1000.0f / av_q2d(st->time_base));
    pkt.duration = 1024;
    pkt.stream_index = st->index;
    AVPacket new_packet;
    av_init_packet(&new_packet);
    ret = av_bitstream_filter_filter(bit_stream_filter_context_, st->codec, nullptr,
            &new_packet.data, &new_packet.size,
            pkt.data, pkt.size, pkt.flags & AV_PKT_FLAG_KEY);
    if (ret >= 0) {
        new_packet.pts = pkt.pts;
        new_packet.dts = pkt.dts;
        new_packet.duration = pkt.duration;
        new_packet.stream_index = pkt.stream_index;
        ret = av_interleaved_write_frame(oc, &new_packet);
        if (ret != 0) {
            LOGE("write audio frame error: %s", av_err2str(ret));
        }
    } else {
        LOGE("av_bitstream_filter_filter: %s", av_err2str(ret));
    }
    av_packet_unref(&new_packet);
    av_packet_unref(&pkt);
    delete audio_packet;
    return ret;
}

uint32_t Mp4Muxer::FindStartCode(uint8_t *in_buffer, uint32_t in_ui32_buffer_size,
                                 uint32_t in_ui32_code, uint32_t &out_ui32_processed_bytes) {
    uint32_t ui32Code = in_ui32_code;
    const uint8_t * ptr = in_buffer;
    while (ptr < in_buffer + in_ui32_buffer_size) {
        ui32Code = *ptr++ + (ui32Code << 8);
        if (is_start_code(ui32Code)) {
            break;
        }
    }
    out_ui32_processed_bytes = (uint32_t)(ptr - in_buffer);
    return ui32Code;
}

void Mp4Muxer::ParseH264SequenceHeader(uint8_t *in_buffer, uint32_t in_ui32_size,
                                       uint8_t **in_sps_buffer, int &in_sps_size,
                                       uint8_t **in_pps_buffer, int &in_pps_size) {
    uint32_t ui32StartCode = 0x0ff;
    uint8_t* pBuffer = in_buffer;
    uint32_t ui32BufferSize = in_ui32_size;
    uint32_t sps = 0;
    uint32_t pps = 0;
    uint32_t idr = in_ui32_size;
    do {
        uint32_t ui32ProcessedBytes = 0;
        ui32StartCode = FindStartCode(pBuffer, ui32BufferSize, ui32StartCode,
                                      ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;
        if (ui32BufferSize < 1) {
            break;
        }

        uint8_t val = static_cast<uint8_t>(*pBuffer & 0x1f);
        // idr
        if (val == 5) {
            idr = pps + ui32ProcessedBytes - 4;
        }
        // sps
        if (val == 7) {
            sps = ui32ProcessedBytes;
        }
        // pps
        if (val == 8) {
            pps = sps + ui32ProcessedBytes;
        }
    } while (ui32BufferSize > 0);
    *in_sps_buffer = in_buffer + sps - 4;
    in_sps_size = pps - sps;
    *in_pps_buffer = in_buffer + pps - 4;
    in_pps_size = idr - pps + 4;
}

int Mp4Muxer::WriteVideoFrame(AVFormatContext *oc, AVStream *st, bool wait) {
    int ret = 0;
    AVCodecContext *c = st->codec;

    VideoPacket *h264Packet = nullptr;
    video_packet_callback_(&h264Packet, video_packet_context_, wait);
    if (h264Packet == nullptr) {
        LOGE("fill_h264_packet_callback_ Get null packet_");
        return VIDEO_QUEUE_ABORT_ERR_CODE;
    }
    int bufferSize = (h264Packet)->size;
    uint8_t* outputData = h264Packet->buffer;
    last_video_presentation_time_ms_ = h264Packet->timeMills;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    pkt.stream_index = st->index;
    int64_t cal_pts = static_cast<int64_t>(last_video_presentation_time_ms_ / 1000.0f /
                                           av_q2d(video_stream_->time_base));
    int64_t pts = h264Packet->pts == PTS_PARAM_UN_SETTIED_FLAG ? cal_pts : h264Packet->pts;
    int64_t dts = h264Packet->dts == DTS_PARAM_UN_SETTIED_FLAG ? pts : h264Packet->dts == DTS_PARAM_NOT_A_NUM_FLAG ? AV_NOPTS_VALUE : h264Packet->dts;
    int nalu_type = (outputData[4] & 0x1F);
    if (nalu_type == H264_NALU_TYPE_SEQUENCE_PARAMETER_SET) {
        // 我们这里要求sps和pps一块拼接起来构造成AVPacket传过来
        header_size_ = bufferSize;
        header_data_ = new uint8_t[header_size_];
        memcpy(header_data_, outputData, static_cast<size_t>(bufferSize));

        uint8_t* spsFrame = 0;
        uint8_t* ppsFrame = 0;

        int spsFrameLen = 0;
        int ppsFrameLen = 0;

        ParseH264SequenceHeader(header_data_, static_cast<uint32_t>(header_size_), &spsFrame, spsFrameLen,
                                &ppsFrame, ppsFrameLen);

        // Extradata contains PPS & SPS for AVCC format
        int extradata_len = 8 + spsFrameLen - 4 + 1 + 2 + ppsFrameLen - 4;
        c->extradata = reinterpret_cast<uint8_t*>(av_mallocz(static_cast<size_t>(extradata_len)));
        c->extradata_size = extradata_len;
        c->extradata[0] = 0x01;
        c->extradata[1] = spsFrame[4 + 1];
        c->extradata[2] = spsFrame[4 + 2];
        c->extradata[3] = spsFrame[4 + 3];
        c->extradata[4] = 0xFC | 3;
        c->extradata[5] = 0xE0 | 1;
        int tmp = spsFrameLen - 4;
        c->extradata[6] = static_cast<uint8_t>((tmp >> 8) & 0x00ff);
        c->extradata[7] = static_cast<uint8_t>(tmp & 0x00ff);
        int i = 0;
        for (i = 0; i < tmp; i++) {
            c->extradata[8 + i] = spsFrame[4 + i];
        }
        c->extradata[8 + tmp] = 0x01;
        int tmp2 = ppsFrameLen - 4;
        c->extradata[8 + tmp + 1] = static_cast<uint8_t>((tmp2 >> 8) & 0x00ff);
        c->extradata[8 + tmp + 2] = static_cast<uint8_t>(tmp2 & 0x00ff);
        for (i = 0; i < tmp2; i++) {
            c->extradata[8 + tmp + 3 + i] = ppsFrame[4 + i];
        }

        ret = avformat_write_header(oc, nullptr);
        if (ret < 0) {
            LOGE("Error occurred when opening output file: %s\n", av_err2str(ret));
        } else {
            write_header_success_ = true;
        }
    } else {
        if (nalu_type == H264_NALU_TYPE_IDR_PICTURE || nalu_type == H264_NALU_TYPE_SEI) {
            pkt.size = bufferSize;
            pkt.data = outputData;

            if (pkt.data[0] == 0x00 && pkt.data[1] == 0x00 &&
                pkt.data[2] == 0x00 && pkt.data[3] == 0x01) {
                bufferSize -= 4;
                pkt.data[0] = static_cast<uint8_t>(((bufferSize) >> 24) & 0x00ff);
                pkt.data[1] = static_cast<uint8_t>(((bufferSize) >> 16) & 0x00ff);
                pkt.data[2] = static_cast<uint8_t>(((bufferSize) >> 8) & 0x00ff);
                pkt.data[3] = static_cast<uint8_t>(((bufferSize)) & 0x00ff);
            }

            pkt.pts = pts;
            pkt.dts = dts;
            pkt.flags = AV_PKT_FLAG_KEY;
            c->frame_number++;
        } else {
            pkt.size = bufferSize;
            pkt.data = outputData;

            if (pkt.data[0] == 0x00 && pkt.data[1] == 0x00 &&
                pkt.data[2] == 0x00 && pkt.data[3] == 0x01) {
                bufferSize -= 4;
                pkt.data[0] = static_cast<uint8_t>(((bufferSize) >> 24) & 0x00ff);
                pkt.data[1] = static_cast<uint8_t>(((bufferSize) >> 16) & 0x00ff);
                pkt.data[2] = static_cast<uint8_t>(((bufferSize) >> 8) & 0x00ff);
                pkt.data[3] = static_cast<uint8_t>(((bufferSize)) & 0x00ff);
            }

            pkt.pts = pts;
            pkt.dts = dts;
            pkt.flags = 0;
            c->frame_number++;
        }
        if (pkt.size) {
            ret = av_interleaved_write_frame(oc, &pkt);
            if (ret != 0) {
                LOGE("write frame error: %d", ret);
            }
        } else {
            ret = 0;
        }
    }
    av_packet_unref(&pkt);
    delete h264Packet;
    return ret;
}

double Mp4Muxer::GetVideoStreamTimeInSecs() {
    return last_video_presentation_time_ms_ / 1000.0f;
}

void Mp4Muxer::CloseVideo(AVFormatContext *oc, AVStream *st) {
    if (nullptr != st->codec) {
        avcodec_close(st->codec);
    }
}

void Mp4Muxer::CloseAudio(AVFormatContext *oc, AVStream *st) {
    if (nullptr != st->codec) {
        avcodec_close(st->codec);
    }
    if (nullptr != bit_stream_filter_context_) {
        av_bitstream_filter_close(bit_stream_filter_context_);
    }
}

double Mp4Muxer::GetAudioStreamTimeInSecs() {
    return last_audio_packet_presentation_time_mills_ / 1000.0f;
}

int Mp4Muxer::BuildVideoStream() {
    AVCodec* codec = nullptr;
    video_stream_ = AddStream(format_context_, &codec, AV_CODEC_ID_H264, nullptr);
    return 0;
}

int GetSampleRateIndex(unsigned int sampling_frequency) {
    switch (sampling_frequency) {
        case 96000:
            return 0;
        case 88200:
            return 1;
        case 64000:
            return 2;
        case 48000:
            return 3;
        case 44100:
            return 4;
        case 32000:
            return 5;
        case 24000:
            return 6;
        case 22050:
            return 7;
        case 16000:
            return 8;
        case 12000:
            return 9;
        case 11025:
            return 10;
        case 8000:
            return 11;
        case 7350:
            return 12;
        default:
            return 0;
    }
}

int Mp4Muxer::BuildAudioStream(char *audio_codec_name) {
    AVCodec* codec = nullptr;
    audio_stream_ = AddStream(format_context_, &codec, AV_CODEC_ID_NONE, audio_codec_name);
    if (nullptr != audio_stream_ && nullptr != codec) {
        AVCodecContext* context = audio_stream_->codec;
        context->extradata = reinterpret_cast<uint8_t*>(av_malloc(2));
        context->extradata_size = 2;
        // AAC LC by default
        unsigned int object_type = 2;
        char dsi[2];
        dsi[0] = static_cast<char>((object_type << 3) |
                                   (GetSampleRateIndex(context->sample_rate) >> 1));
        dsi[1] = static_cast<char>(((GetSampleRateIndex(context->sample_rate) & 1) << 7) |
                                   (context->channels << 3));
        memcpy(context->extradata, dsi, 2);
//        bit_stream_filter_ = av_bsf_get_by_name("aac_adtstoasc");
        bit_stream_filter_context_ = av_bitstream_filter_init("aac_adtstoasc");
    }
    return 0;
}

}  // namespace trinity
