//
// Created by wlanjie on 2019/4/18.
//

#include "mp4_muxer.h"
#include "android_xlog.h"
#include "tools.h"

namespace trinity {

Mp4Muxer::Mp4Muxer() {
    header_data_ = nullptr;
    header_size_ = 0;
    format_context_ = nullptr;
    video_stream_ = nullptr;
    audio_stream_ = nullptr;
    bit_stream_filter_context_ = nullptr;
    duration_ = 0;
    last_audio_packet_presentation_time_mills_ = 0;
    video_width_ = 0;
    video_height_ = 0;
    video_frame_rate_ = 0;
    video_bit_rate_ = 0;
    audio_sample_rate_ = 0;
    audio_channels_ = 0;
    audio_bit_rate_ = 0;
    write_header_success_ = false;
}

Mp4Muxer::~Mp4Muxer() {

}

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
    ret = BuildAudioStream(audio_codec_name);
    if (ret != 0) {
        LOGE("add audio stream error: %d", ret);
        return ret;
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
    if (write_header_success_) {
        av_write_trailer(format_context_);
        format_context_->duration = duration_ * AV_TIME_BASE;
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
    return 0;
}

AVStream *
Mp4Muxer::AddStream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, char *codec_name) {
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
            stream->avg_frame_rate.num = 30000;
            stream->avg_frame_rate.den = (int) (30000 / video_frame_rate_);
            context->time_base.num = 30000;
            context->time_base.den = (int) (30000 / video_frame_rate_);
            context->gop_size = (int) video_frame_rate_;
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

int Mp4Muxer::WriteAudioFrame(AVFormatContext *oc, AVStream *st) {
    AudioPacket* audio_packet = nullptr;
    int ret = audio_packet_callback_(&audio_packet, audio_packet_context_);
    if (ret < 0) {
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
    av_free_packet(&new_packet);
    av_free_packet(&pkt);
    delete audio_packet;
    return ret;
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
        context->extradata = (uint8_t*) av_malloc(2);
        context->extradata_size = 2;
        unsigned int object_type = 2; // AAC LC by default
        char dsi[2];
        dsi[0] = (object_type << 3) | (GetSampleRateIndex(context->sample_rate) >> 1);
        dsi[1] = ((GetSampleRateIndex(context->sample_rate) & 1) << 7) | (context->channels << 3);
        memcpy(context->extradata, dsi, 2);
        bit_stream_filter_context_ = av_bitstream_filter_init("aac_adtstoasc");
    }
    return 0;
}

}