#include "mp4_mux.h"

#define LOG_TAG "RecordingPublisher"

bool verboseOn = false;

Mp4Mux::Mp4Mux() {
    write_header_success_ = false;
    video_stream_ = NULL;
    audio_stream_ = NULL;
    bit_stream_filter_context_ = NULL;
    format_context_ = NULL;
    publishTimeout = 0;
    last_audio_packet_presentation_time_mills_ = 0;
}

Mp4Mux::~Mp4Mux() {
    publishTimeout = 0;
}

void Mp4Mux::registerFillAACPacketCallback(
        int (*fill_aac_packet_callback)(LiveAudioPacket **, void *context), void *context) {
    this->fill_aac_packet_callback_ = fill_aac_packet_callback;
    this->fill_aac_packet_context_ = context;
}

void Mp4Mux::registerFillVideoPacketCallback(
        int (*fill_packet_frame)(LiveVideoPacket **, void *context), void *context) {
    this->fill_h264_packet_callback_ = fill_packet_frame;
    this->fill_h264_packet_context_ = context;
}

int Mp4Mux::detectTimeout() {
    // TODO delete
    return 0;
}

int Mp4Mux::interrupt_cb(void *ctx) {
    Mp4Mux *publisher = (Mp4Mux *) ctx;
    return publisher->detectTimeout();
}

int Mp4Mux::Init(char *videoOutputURI,
                             int videoWidth, int videoHeight, float videoFrameRate,
                             int videoBitRate,
                             int audioSampleRate, int audioChannels, int audioBitRate,
                             char *audio_codec_name,
                             int qualityStrategy) {
    int ret = 0;
    this->duration_ = 0.0;
    this->video_stream_ = NULL;
    this->audio_stream_ = NULL;
    this->video_width_ = videoWidth;
    this->video_height_ = videoHeight;
    this->video_frame_rate_ = videoFrameRate;
    this->video_bit_rate_ = videoBitRate;
    this->audio_sample_rate_ = audioSampleRate;
    this->audio_channels_ = audioChannels;
    this->audio_bit_rate_ = audioBitRate;
    avcodec_register_all();
    av_register_all();
    avformat_network_init();
    LOGI("Publish URL %s", videoOutputURI);
    /* 2:allocate the output media context_ */
    avformat_alloc_output_context2(&format_context_, NULL, "flv", videoOutputURI);
    if (!format_context_) {
        return -1;
    }
    output_format_ = format_context_->oformat;

    /* 3:add Video Stream and Audio Stream to Our AVFormatContext  */
    if ((ret = BuildVideoStream()) < 0) {
        LOGI("BuildVideoStream failed....\n");
        return -1;
    }
    if ((ret = BuildAudioStream(audio_codec_name)) < 0) {
        LOGI("BuildAudioStream failed....\n");
        return -1;
    }

    /* 4:open the local file Handle or remote file Handle, if needed */
    if (!(output_format_->flags & AVFMT_NOFILE)) {
        AVIOInterruptCB int_cb = {interrupt_cb, this};
        format_context_->interrupt_callback = int_cb;
        ret = avio_open2(&format_context_->pb, videoOutputURI, AVIO_FLAG_WRITE, &format_context_->interrupt_callback,
                         NULL);
        if (ret < 0) {
            LOGI("Could not open '%s': %s\n", videoOutputURI, av_err2str(ret));
            return -1;
        }
    } else {
        return -1;
    }

    return 1;
}

int Mp4Mux::Encode() {
    int ret = 0;
    /* Compute current audio and video time. */
    double video_time = GetVideoStreamTimeInSecs();
    double audio_time = GetAudioStreamTimeInSecs();
    if (verboseOn) {
        LOGI("video_time is %lf, audio_time is %f", video_time, audio_time);
    }
    /* write interleaved audio and video frames */
    if (!video_stream_ || (video_stream_ && audio_stream_ && audio_time < video_time)) {
        ret = WriteAudioFrame(format_context_, audio_stream_);
    } else if (video_stream_) {
        ret = WriteVideoFrame(format_context_, video_stream_);
    }
    duration_ = MIN(audio_time, video_time);
    return ret;
}

int Mp4Mux::Stop() {
    LOGI("enter Mp4Mux::Stop...");
    int ret = 0;
    if (write_header_success_) {
        LOGI("leave Mp4Mux::Stop() if (isConnected && write_header_success_)");
        av_write_trailer(format_context_);
        //设置视频长度
        format_context_->duration = duration_ * AV_TIME_BASE;
    }

    /* Close each codec 防止内存泄露. */
    if (video_stream_) {
        CloseVideo(format_context_, video_stream_);
        video_stream_ = NULL;
    }
    if (audio_stream_) {
        CloseAudio(format_context_, audio_stream_);
        audio_stream_ = NULL;
    }

    // TODO avio_close
    if (!(output_format_->flags & AVFMT_NOFILE))
        avio_close(format_context_->pb);

    if (format_context_) {
        avformat_free_context(format_context_);
        format_context_ = NULL;
    }
    LOGI("leave Mp4Mux::Stop...");
    return ret;
}

int Mp4Mux::BuildAudioStream(char *audio_codec_name) {
    int ret = 1;
    AVCodec *audio_codec = NULL;
    audio_stream_ = AddStream(format_context_, &audio_codec, AV_CODEC_ID_NONE, audio_codec_name);
    if (audio_stream_ && audio_codec) {
        if ((ret = OpenAudio(format_context_, audio_codec, audio_stream_)) < 0) {
            LOGI("OpenAudio failed....\n");
            return ret;
        }
    }
    return ret;
}

int Mp4Mux::BuildVideoStream() {
    int ret = 1;
    AVCodec *video_codec = NULL;
    output_format_->video_codec = AV_CODEC_ID_H264;
    if (output_format_->video_codec != AV_CODEC_ID_NONE) {
        video_stream_ = AddStream(format_context_, &video_codec, output_format_->video_codec, NULL);
    }
    if (video_stream_ && video_codec) {
        if ((ret = OpenVideo(format_context_, video_codec, video_stream_)) < 0) {
            LOGI("OpenVideo failed....\n");
            return ret;
        }
    }
    return ret;
}

AVStream *Mp4Mux::AddStream(AVFormatContext *oc, AVCodec **codec,
                            enum AVCodecID codec_id, char *codec_name) {
    AVCodecContext *c;
    AVStream *st;
    if (AV_CODEC_ID_NONE == codec_id) {
        *codec = avcodec_find_encoder_by_name(codec_name);
    } else {
        *codec = avcodec_find_encoder(codec_id);
    }
    if (!(*codec)) {
        LOGI("Could not find encoder_ for '%s'\n", avcodec_get_name(codec_id));
        return NULL;
    }
    if (verboseOn)
        LOGI("\n find encoder_ name is '%s' ", (*codec)->name);

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        LOGI("Could not allocate stream\n");
        return NULL;
    }
    st->id = oc->nb_streams - 1;
    c = st->codec;

    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
            LOGI("audio_bit_rate_ is %d channels_ is %d sample_rate_ is %d", audio_bit_rate_,
                 audio_channels_, audio_sample_rate_);
            c->sample_fmt = AV_SAMPLE_FMT_S16;
            c->bit_rate = audio_bit_rate_;
            c->codec_type = AVMEDIA_TYPE_AUDIO;
            c->sample_rate = audio_sample_rate_;
            c->channel_layout = audio_channels_ == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
            c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
            c->flags |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        case AVMEDIA_TYPE_VIDEO:
            c->codec_id = AV_CODEC_ID_H264;
            c->bit_rate = video_bit_rate_;
            c->width = video_width_;
            c->height = video_height_;

            st->avg_frame_rate.num = 30000;
            st->avg_frame_rate.den = (int) (30000 / video_frame_rate_);

            c->time_base.den = 30000;
            c->time_base.num = (int) (30000 / video_frame_rate_);
            /* gop_size 设置图像组大小 这里设置GOP大小，也表示两个I帧之间的间隔 */
            c->gop_size = video_frame_rate_;
            /**  -qscale q  使用固定的视频量化标度(VBR)  以<q>质量为基础的VBR，取值0.01-255，约小质量越好，即qscale 4和-qscale 6，4的质量比6高 。
             * 					此参数使用次数较多，实际使用时发现，qscale是种固定量化因子，设置qscale之后，前面设置的-b好像就无效了，而是自动调整了比特率。
             *	 -qmin q 最小视频量化标度(VBR) 设定最小质量，与-qmax（设定最大质量）共用
             *	 -qmax q 最大视频量化标度(VBR) 使用了该参数，就可以不使用qscale参数  **/
            c->qmin = 10;
            c->qmax = 30;
            c->pix_fmt = COLOR_FORMAT;
            // 新增语句，设置为编码延迟
            av_opt_set(c->priv_data, "preset", "ultrafast", 0);
            // 实时编码关键看这句，上面那条无所谓
            av_opt_set(c->priv_data, "tune", "zerolatency", 0);

            /* Some formats want stream headers to be separate. */
            if (oc->oformat->flags & AVFMT_GLOBALHEADER)
                c->flags |= CODEC_FLAG_GLOBAL_HEADER;

            if (verboseOn)
                LOGI("sample_aspect_ratio = %d   %d", c->sample_aspect_ratio.den,
                     c->sample_aspect_ratio.num);
            break;
        default:
            break;
    }
    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    return st;
}

/** 2、开启视频编码器 并且分配可重用的buffer **/
int Mp4Mux::OpenVideo(AVFormatContext *oc, AVCodec *codec, AVStream *st) {
    //DO Nothing
    return 1;
}

int get_sr_index(unsigned int sampling_frequency) {
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

/** 3、开启音频编码器 并且分配可重复用的audio frame **/
int Mp4Mux::OpenAudio(AVFormatContext *oc, AVCodec *codec, AVStream *st) {
    AVCodecContext *c = st->codec;
    c->extradata = (uint8_t *) av_malloc(2);
    c->extradata_size = 2;
    //下面这一行是单声道
    //memcpy(c->extradata, "\x12\x08", 2);
    //下面这一行是双声道
//    memcpy(c->extradata, "\x12\x10", 2);
    unsigned int object_type = 2; // AAC LC by default
    char dsi[2];
    dsi[0] = (object_type << 3) | (get_sr_index(c->sample_rate) >> 1);
    dsi[1] = ((get_sr_index(c->sample_rate) & 1) << 7) | (c->channels << 3);
    memcpy(c->extradata, dsi, 2);
    bit_stream_filter_context_ = av_bitstream_filter_init("aac_adtstoasc");
    return 1;
}

double Mp4Mux::GetAudioStreamTimeInSecs() {
    return last_audio_packet_presentation_time_mills_ / 1000.0f;
}

/** 5、为音频流写入一帧数据 **/
int Mp4Mux::WriteAudioFrame(AVFormatContext *oc, AVStream *st) {
    int ret = AUDIO_QUEUE_ABORT_ERR_CODE;
    LiveAudioPacket *audioPacket = NULL;
    if ((ret = fill_aac_packet_callback_(&audioPacket, fill_aac_packet_context_)) > 0) {
        AVPacket pkt = {0}; // data and Size must be 0;
        av_init_packet(&pkt);
        last_audio_packet_presentation_time_mills_ = audioPacket->position;
        pkt.data = audioPacket->data;
        pkt.size = audioPacket->size;
        /** 3、给编码后的packet计算正确的pts/dts/duration/stream_index **/
        pkt.dts = pkt.pts = last_audio_packet_presentation_time_mills_ / 1000.0f / av_q2d(st->time_base);
        pkt.duration = 1024;
//		pkt_.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = st->index;
        AVPacket newpacket;
        av_init_packet(&newpacket);
        ret = av_bitstream_filter_filter(bit_stream_filter_context_, st->codec, NULL, &newpacket.data, &newpacket.size,
                                         pkt.data, pkt.size, pkt.flags & AV_PKT_FLAG_KEY);
        if (ret >= 0) {
            /** 4、将编码后的packet数据写入文件 **/
            newpacket.pts = pkt.pts;
            newpacket.dts = pkt.dts;
            newpacket.duration = pkt.duration;
            newpacket.stream_index = pkt.stream_index;
            ret = this->InterleavedWriteFrame(oc, &newpacket);
            if (ret != 0) {
                LOGI("Error while writing audio frame_: %s\n", av_err2str(ret));
            }
        } else {
            LOGI("Error av_bitstream_filter_filter: %s\n", av_err2str(ret));
        }
        av_free_packet(&newpacket);
        av_free_packet(&pkt);
        delete audioPacket;
    } else {
        ret = AUDIO_QUEUE_ABORT_ERR_CODE;
    }
    return ret;
}

int Mp4Mux::InterleavedWriteFrame(AVFormatContext *s, AVPacket *pkt) {
    return av_interleaved_write_frame(s, pkt);
}

/** 6、关闭视频流 **/
void Mp4Mux::CloseVideo(AVFormatContext *oc, AVStream *st) {
    if (NULL != st->codec) {
        avcodec_close(st->codec);
    }
}

/** 7、关闭音频流 **/
void Mp4Mux::CloseAudio(AVFormatContext *oc, AVStream *st) {
    if (NULL != st->codec) {
        avcodec_close(st->codec);
    }
    if (NULL != bit_stream_filter_context_) {
        av_bitstream_filter_close(bit_stream_filter_context_);
    }
}
