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
      video_codec_context_(nullptr),
      audio_stream_(nullptr),
      audio_codec_context_(nullptr),
      bsf_context_(nullptr),
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
                            int audio_sample_rate, int audio_channels, int audio_bit_rate, std::string& audio_codec_name) {
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

void Mp4Muxer::RegisterAudioPacketCallback(int (*audio_packet)(AudioPacket **, void *context), void *context) {
    audio_packet_callback_ = audio_packet;
    audio_packet_context_ = context;
}

void Mp4Muxer::RegisterVideoPacketCallback(int (*video_packet)(VideoPacket **, void *context), void *context) {
    video_packet_callback_ = video_packet;
    video_packet_context_ = context;
}

int Mp4Muxer::Encode() {
    double video_time = GetVideoStreamTimeInSecs();
    double audio_time = GetAudioStreamTimeInSecs();
    int ret = 0;
    /* write interleaved audio and video frames */
    if (!video_stream_ || (video_stream_ && audio_stream_ && audio_time < video_time)) {
        ret = WriteAudioFrame(format_context_, audio_stream_);
    } else if (video_stream_) {
        ret = WriteVideoFrame(format_context_, video_stream_);
    }
    duration_ = MIN(audio_time, video_time);
    return ret;
}

int Mp4Muxer::Stop() {
    LOGE("enter: %s", __func__);
    // flush audio
//    if (nullptr != audio_stream_) {
//        int ret = WriteAudioFrame(format_context_, audio_stream_);
//        while (ret >= 0) {
//            ret = WriteAudioFrame(format_context_, audio_stream_);
//        }
//    }
//    // flush video
//    if (nullptr != video_stream_) {
//        int ret = WriteVideoFrame(format_context_, video_stream_);
//        while (ret >= 0) {
//            ret = WriteVideoFrame(format_context_, video_stream_);
//        }
//    }
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

int Mp4Muxer::WriteAudioFrame(AVFormatContext *oc, AVStream *st) {
    if (nullptr == bsf_context_) {
        LOGE("nullptr == bsf_context_");
        return -1;
    }
    AudioPacket* audio_packet = nullptr;
    int ret = audio_packet_callback_(&audio_packet, audio_packet_context_);
    if (ret < 0 || audio_packet == nullptr) {
        LOGE("audio_packet_callback_ ret: %d", ret);
        return ret;
    }
    AVPacket* pkt = av_packet_alloc();
    last_audio_packet_presentation_time_mills_ = audio_packet->position;
    pkt->data = audio_packet->data;
    pkt->size = audio_packet->size;
    pkt->dts = pkt->pts = (int64_t) (last_audio_packet_presentation_time_mills_ / 1000.0f / av_q2d(st->time_base));
    pkt->duration = 1024;
    pkt->stream_index = st->index;
    ret = av_bsf_send_packet(bsf_context_, pkt);
    if (ret < 0) {
        LOGE("av_bsf_send_packet error: %s", av_err2str(ret));
        av_packet_free(&pkt) ;
        delete audio_packet;
        return -2;
    }
    do {
        ret = av_bsf_receive_packet(bsf_context_, pkt);
        if (ret < 0) {
            break;
        }
        ret = av_interleaved_write_frame(oc, pkt);
        if (ret != 0) {
            LOGE("write audio frame error: %s", av_err2str(ret));
        }
    } while (true);

    av_packet_free(&pkt);
    delete audio_packet;
    return 0;
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

int Mp4Muxer::WriteVideoFrame(AVFormatContext *oc, AVStream *st) {
    int ret = 0;
    AVCodecParameters* c = st->codecpar;

    VideoPacket *h264Packet = nullptr;
    video_packet_callback_(&h264Packet, video_packet_context_);
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
    if (nullptr != video_codec_context_) {
        avcodec_close(video_codec_context_);
        video_codec_context_ = nullptr;
    }
}

void Mp4Muxer::CloseAudio(AVFormatContext *oc, AVStream *st) {
    if (nullptr != audio_codec_context_) {
        avcodec_close(audio_codec_context_);
        audio_codec_context_ = nullptr;
    }
    if (nullptr != bsf_context_) {
        av_bsf_free(&bsf_context_);
        bsf_context_ = nullptr;
    }
}

double Mp4Muxer::GetAudioStreamTimeInSecs() {
    return last_audio_packet_presentation_time_mills_ / 1000.0f;
}

int Mp4Muxer::BuildVideoStream() {
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (nullptr == codec) {
        LOGE("h264 encode codec is null");
        return -1;
    }
    video_stream_ = avformat_new_stream(format_context_, codec);
    if (nullptr == video_stream_) {
        return -2;
    }
    video_codec_context_ = avcodec_alloc_context3(nullptr);
    if (nullptr == video_codec_context_) {
        return -3;
    }
    video_codec_context_->codec_type = AVMEDIA_TYPE_VIDEO;
    video_codec_context_->codec_id = AV_CODEC_ID_H264;
    video_codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
    video_codec_context_->bit_rate = video_bit_rate_;
    video_codec_context_->width = video_width_;
    video_codec_context_->height = video_height_;
    AVRational video_time_base = { 1, 10000 };
    video_codec_context_->time_base = video_time_base;
    video_stream_->avg_frame_rate = video_time_base;
    video_stream_->time_base = video_time_base;
    video_codec_context_->gop_size = static_cast<int>(video_frame_rate_);
    video_codec_context_->qmin = 10;
    video_codec_context_->qmax = 30;
    // 新增语句，设置为编码延迟
    av_opt_set(video_codec_context_->priv_data, "preset", "ultrafast", 0);
    // 实时编码关键看这句，上面那条无所谓
    av_opt_set(video_codec_context_->priv_data, "tune", "zerolatency", 0);
    if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
        video_codec_context_->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_parameters_from_context(video_stream_->codecpar, video_codec_context_);
    return 0;
}

int GetSampleRateIndex(int sampling_frequency) {
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

int Mp4Muxer::BuildAudioStream(std::string& audio_codec_name) {
    LOGI("enter: %s", __func__);
    AVCodec* codec = avcodec_find_encoder_by_name(audio_codec_name.c_str());
    if (nullptr == codec) {
        LOGE("find audio encode name error: %s", audio_codec_name.c_str());
        return -1;
    }
    audio_stream_ = avformat_new_stream(format_context_, codec);
    if (nullptr == audio_stream_) {
        LOGE("new audio stream error");
        return -2;
    }
    audio_codec_context_ = avcodec_alloc_context3(nullptr);
    audio_codec_context_->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_codec_context_->sample_fmt = AV_SAMPLE_FMT_S16;
    audio_codec_context_->codec_id = AV_CODEC_ID_AAC;
    audio_codec_context_->sample_rate = audio_sample_rate_;
    audio_codec_context_->bit_rate = audio_bit_rate_;
    audio_codec_context_->channel_layout = audio_channels_ == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
    audio_codec_context_->channels = av_get_channel_layout_nb_channels(audio_codec_context_->channel_layout);
    audio_codec_context_->flags |= CODEC_FLAG_GLOBAL_HEADER;
    audio_codec_context_->extradata = reinterpret_cast<uint8_t*>(av_malloc(2));
    audio_codec_context_->extradata_size = 2;
    // AAC LC by default
    unsigned int object_type = 2;
    uint8_t dsi[2];
    dsi[0] = static_cast<uint8_t>((object_type << 3) |
                               (GetSampleRateIndex(audio_codec_context_->sample_rate) >> 1));
    dsi[1] = static_cast<uint8_t>(((GetSampleRateIndex(audio_codec_context_->sample_rate) & 1) << 7) |
                               (audio_codec_context_->channels << 3));
    memcpy(audio_codec_context_->extradata, dsi, 2);
    avcodec_parameters_from_context(audio_stream_->codecpar, audio_codec_context_);
    const AVBitStreamFilter* bit_stream_filter_ = av_bsf_get_by_name("aac_adtstoasc");
    if (nullptr == bit_stream_filter_) {
        LOGE("aac adts filter is null.");
        return -1;
    }
    int ret = av_bsf_alloc(bit_stream_filter_, &bsf_context_);
    if (ret < 0) {
        LOGE("av_bsf_alloc error: %s", av_err2str(ret));
        return ret;
    }
    ret = avcodec_parameters_copy(bsf_context_->par_in, audio_stream_->codecpar);
    if (ret < 0) {
        LOGE("copy parameters to par_in error: %s", av_err2str(ret));
        return ret;
    }
    ret = av_bsf_init(bsf_context_);
    if (ret < 0) {
        LOGE("av_bsf_init error: %s", av_err2str(ret));
        return ret;
    }
    LOGI("leave: %s", __func__);
    return 0;
}

}  // namespace trinity
