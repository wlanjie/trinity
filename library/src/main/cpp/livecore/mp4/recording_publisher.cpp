#include "./recording_publisher.h"

#define LOG_TAG "RecordingPublisher"

bool verboseOn = false;

RecordingPublisher::RecordingPublisher() {
    isWriteHeaderSuccess = false;
    video_st = NULL;
    audio_st = NULL;
    bsfc = NULL;
    oc = NULL;
    publishTimeout = 0;
    lastAudioPacketPresentationTimeMills = 0;
}

RecordingPublisher::~RecordingPublisher() {
    publishTimeout = 0;
}

void RecordingPublisher::registerPublishTimeoutCallback(
        int (*on_publish_timeout_callback)(void *context), void *context) {
    // TODO delete
}

void RecordingPublisher::registerHotAdaptiveBitrateCallback(
        int (*hot_adaptive_bitrate_callback)(int maxBitrate, int avgBitrate, int fps,
                                             void *context),
        void *context) {
}

void RecordingPublisher::registerStatisticsBitrateCallback(
        int (*statistics_bitrate_callback)(int sendBitrate, int compressedBitrate,
                                           void *context),
        void *context) {
}

void RecordingPublisher::registerFillAACPacketCallback(
        int (*fill_aac_packet_callback)(LiveAudioPacket **, void *context), void *context) {
    this->fillAACPacketCallback = fill_aac_packet_callback;
    this->fillAACPacketContext = context;
}

void RecordingPublisher::registerFillVideoPacketCallback(
        int (*fill_packet_frame)(LiveVideoPacket **, void *context), void *context) {
    this->fillH264PacketCallback = fill_packet_frame;
    this->fillH264PacketContext = context;
}

int RecordingPublisher::detectTimeout() {
    // TODO delete
    return 0;
}

int RecordingPublisher::interrupt_cb(void *ctx) {
    RecordingPublisher *publisher = (RecordingPublisher *) ctx;
    return publisher->detectTimeout();
}

int RecordingPublisher::init(char *videoOutputURI,
                             int videoWidth, int videoHeight, float videoFrameRate,
                             int videoBitRate,
                             int audioSampleRate, int audioChannels, int audioBitRate,
                             char *audio_codec_name,
                             int qualityStrategy,
							 const std::map<std::string, int>& configMap) {
    int ret = 0;
    this->publishTimeout = PUBLISH_DATA_TIME_OUT;
    this->duration = 0.0;
    this->video_st = NULL;
    this->audio_st = NULL;
    this->videoWidth = videoWidth;
    this->videoHeight = videoHeight;
    this->videoFrameRate = videoFrameRate;
    this->videoBitRate = videoBitRate;
    this->audioSampleRate = audioSampleRate;
    this->audioChannels = audioChannels;
    this->audioBitRate = audioBitRate;
    /* 1:Initialize libavcodec, and register all codecs and formats. */
    avcodec_register_all();
    av_register_all();
    avformat_network_init();
    LOGI("Publish URL %s", videoOutputURI);
    /* 2:allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, "flv", videoOutputURI);
    if (!oc) {
        return -1;
    }
    fmt = oc->oformat;

    /* 3:add Video Stream and Audio Stream to Our AVFormatContext  */
    if ((ret = buildVideoStream()) < 0) {
        LOGI("buildVideoStream failed....\n");
        return -1;
    }
    if ((ret = buildAudioStream(audio_codec_name)) < 0) {
        LOGI("buildAudioStream failed....\n");
        return -1;
    }

    /* 4:open the local file Handle or remote file Handle, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (PUBLISH_INVALID_FLAG != this->publishTimeout) {
            AVIOInterruptCB int_cb = {interrupt_cb, this};
            oc->interrupt_callback = int_cb;
            ret = avio_open2(&oc->pb, videoOutputURI, AVIO_FLAG_WRITE, &oc->interrupt_callback,
                             NULL);
            if (ret < 0) {
                LOGI("Could not open '%s': %s\n", videoOutputURI, av_err2str(ret));
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    return 1;
}

int RecordingPublisher::encode() {
    int ret = 0;
    /* Compute current audio and video time. */
    double video_time = getVideoStreamTimeInSecs();
    double audio_time = getAudioStreamTimeInSecs();
    if (verboseOn) {
        LOGI("video_time is %lf, audio_time is %f", video_time, audio_time);
    }
    /* write interleaved audio and video frames */
    if (!video_st || (video_st && audio_st && audio_time < video_time)) {
        ret = write_audio_frame(oc, audio_st);
    } else if (video_st) {
        ret = write_video_frame(oc, video_st);
    }
    duration = MIN(audio_time, video_time);
    return ret;
}

int RecordingPublisher::stop() {
    LOGI("enter RecordingPublisher::stop...");
    int ret = 0;
    if (isWriteHeaderSuccess) {
        LOGI("leave RecordingPublisher::stop() if (isConnected && isWriteHeaderSuccess)");
        av_write_trailer(oc);
        //设置视频长度
        oc->duration = duration * AV_TIME_BASE;
    }

    /* Close each codec 防止内存泄露. */
    if (video_st) {
        close_video(oc, video_st);
        video_st = NULL;
    }
    if (audio_st) {
        close_audio(oc, audio_st);
        audio_st = NULL;
    }

    // TODO avio_close
    if (!(fmt->flags & AVFMT_NOFILE))
        avio_close(oc->pb);

    if (oc) {
        /* free the stream */
        avformat_free_context(oc);
        oc = NULL;
    }
    LOGI("leave RecordingPublisher::stop...");
    return ret;
}

int RecordingPublisher::buildAudioStream(char *audio_codec_name) {
    int ret = 1;
    AVCodec *audio_codec = NULL;
    audio_st = add_stream(oc, &audio_codec, AV_CODEC_ID_NONE, audio_codec_name);
    if (audio_st && audio_codec) {
        if ((ret = open_audio(oc, audio_codec, audio_st)) < 0) {
            LOGI("open_audio failed....\n");
            return ret;
        }
    }
    return ret;
}

int RecordingPublisher::buildVideoStream() {
    int ret = 1;
    /* Add the audio and video streams using the default format codecs
     * and Initialize the codecs. */
    AVCodec *video_codec = NULL;
    //视频部分-->强制指定H264(libx264)编码
    fmt->video_codec = AV_CODEC_ID_H264;
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        video_st = add_stream(oc, &video_codec, fmt->video_codec, NULL);
    }
    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (video_st && video_codec) {
        if ((ret = open_video(oc, video_codec, video_st)) < 0) {
            LOGI("open_video failed....\n");
            return ret;
        }
    }
    return ret;
}

/** 1、为AVFormatContext增加指定的编码器 **/
AVStream *RecordingPublisher::add_stream(AVFormatContext *oc, AVCodec **codec,
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
            LOGI("audioBitRate is %d audioChannels is %d audioSampleRate is %d", audioBitRate,
                 audioChannels, audioSampleRate);
            c->sample_fmt = AV_SAMPLE_FMT_S16;
            c->bit_rate = audioBitRate;
            c->codec_type = AVMEDIA_TYPE_AUDIO;
            c->sample_rate = audioSampleRate;
            c->channel_layout = audioChannels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
            c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
            c->flags |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        case AVMEDIA_TYPE_VIDEO:
            c->codec_id = AV_CODEC_ID_H264;
            c->bit_rate = videoBitRate;
            c->width = videoWidth;
            c->height = videoHeight;

            st->avg_frame_rate.num = 30000;
            st->avg_frame_rate.den = (int) (30000 / videoFrameRate);

            c->time_base.den = 30000;
            c->time_base.num = (int) (30000 / videoFrameRate);
            /* gop_size 设置图像组大小 这里设置GOP大小，也表示两个I帧之间的间隔 */
            c->gop_size = videoFrameRate;
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
int RecordingPublisher::open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st) {
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
int RecordingPublisher::open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st) {
//	int ret;
    AVCodecContext *c = st->codec;
//    ret = avcodec_open2(c, codec, NULL);
//    if (ret < 0) {
//        LOGI("Could not open audio codec: %s\n", av_err2str(ret));
//        return -1;
//    }
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
    bsfc = av_bitstream_filter_init("aac_adtstoasc");

    return 1;
}

double RecordingPublisher::getAudioStreamTimeInSecs() {
    return lastAudioPacketPresentationTimeMills / 1000.0f;
}

/** 5、为音频流写入一帧数据 **/
int RecordingPublisher::write_audio_frame(AVFormatContext *oc, AVStream *st) {
    int ret = AUDIO_QUEUE_ABORT_ERR_CODE;
    LiveAudioPacket *audioPacket = NULL;
    if ((ret = fillAACPacketCallback(&audioPacket, fillAACPacketContext)) > 0) {
        AVPacket pkt = {0}; // data and size must be 0;
        av_init_packet(&pkt);
        lastAudioPacketPresentationTimeMills = audioPacket->position;
        pkt.data = audioPacket->data;
        pkt.size = audioPacket->size;
        /** 3、给编码后的packet计算正确的pts/dts/duration/stream_index **/
        pkt.dts = pkt.pts = lastAudioPacketPresentationTimeMills / 1000.0f / av_q2d(st->time_base);
        pkt.duration = 1024;
//		pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = st->index;
        AVPacket newpacket;
        av_init_packet(&newpacket);
        ret = av_bitstream_filter_filter(bsfc, st->codec, NULL, &newpacket.data, &newpacket.size,
                                         pkt.data, pkt.size, pkt.flags & AV_PKT_FLAG_KEY);
        if (ret >= 0) {
            /** 4、将编码后的packet数据写入文件 **/
            newpacket.pts = pkt.pts;
            newpacket.dts = pkt.dts;
            newpacket.duration = pkt.duration;
            newpacket.stream_index = pkt.stream_index;
            ret = this->interleavedWriteFrame(oc, &newpacket);
            if (ret != 0) {
                LOGI("Error while writing audio frame: %s\n", av_err2str(ret));
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

int RecordingPublisher::interleavedWriteFrame(AVFormatContext *s, AVPacket *pkt) {
    if (startSendTime == 0){
        startSendTime = platform_4_live::getCurrentTimeSeconds();
    }
    int ret = av_interleaved_write_frame(s, pkt);
    return ret;
}

/** 6、关闭视频流 **/
void RecordingPublisher::close_video(AVFormatContext *oc, AVStream *st) {
    if (NULL != st->codec) {
        avcodec_close(st->codec);
    }
}

/** 7、关闭音频流 **/
void RecordingPublisher::close_audio(AVFormatContext *oc, AVStream *st) {
    if (NULL != st->codec) {
        avcodec_close(st->codec);
    }
    if (NULL != bsfc) {
        av_bitstream_filter_close(bsfc);
    }
}
