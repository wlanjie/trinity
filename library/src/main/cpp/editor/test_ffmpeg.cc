//
// Created by wlanjie on 2019-06-19.
//

#include "test_ffmpeg.h"
#include "android_xlog.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

namespace trinity {

FFmpegTest::FFmpegTest() {

}

FFmpegTest::~FFmpegTest() {

}

void FFmpegTest::Transcode() {
    LOGE("Start transcode");
    AVFormatContext* ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Can't allocate context.\n");
        return;
    }
    int ret = avformat_open_input(&ic, "/sdcard/test.mp4", NULL, NULL);
    av_dump_format(ic, 0, "/sdcard/test.mp4", 0);
    AVCodecContext* avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        return;
    }
    ret = avformat_find_stream_info(ic, NULL);
    int video_index = -1;
    for (int i = 0; i < ic->nb_streams; ++i) {
        AVStream* stream = ic->streams[i];
        if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
    }
    ret = avcodec_parameters_to_context(avctx, ic->streams[video_index]->codecpar);
    if (ret < 0) {
        av_err2str(ret);
        avcodec_free_context(&avctx);
        return;
    }
    av_codec_set_pkt_timebase(avctx, ic->streams[video_index]->time_base);
    AVCodec* codec = avcodec_find_decoder(avctx->codec_id);
    avctx->codec_id = codec->id;
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "threads", "auto", 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        avcodec_free_context(&avctx);
        av_err2str(ret);
        return;
    }
    AVPacket pkt1, *pkt = &pkt1;
    AVFrame* frame = av_frame_alloc();
    int got_picture;
    while (1) {
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                break;
            }
        }
        if (pkt->stream_index == video_index) {
            avcodec_decode_video2(avctx, frame, &got_picture, pkt);
            int64_t pts = static_cast<int64_t>(frame->pts * av_q2d(ic->streams[video_index]->time_base) * 1000);
            LOGE("pts: %lld", pts);
        }
    }
    LOGE("end transcode");
}


}