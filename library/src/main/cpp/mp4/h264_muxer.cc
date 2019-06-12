//
// Created by wlanjie on 2019/4/18.
//

#include "h264_muxer.h"
#include "android_xlog.h"

#define is_start_code(code)	(((code) & 0x0ffffff) == 0x01)

namespace trinity {

H264Muxer::H264Muxer() {
    header_data_ = nullptr;
    header_size_ = 0;
    last_presentation_time_ms_ = 0;
}

H264Muxer::~H264Muxer() {

}

int H264Muxer::Stop() {
    int ret = Mp4Muxer::Stop();
    if (nullptr != header_data_) {
        delete[] header_data_;
        header_data_ = nullptr;
    }
    return ret;
}

int H264Muxer::WriteVideoFrame(AVFormatContext *oc, AVStream *st) {
    int ret = 0;
    AVCodecContext *c = st->codec;

    VideoPacket *h264Packet = nullptr;
    video_packet_callback_(&h264Packet, video_packet_context_);
    if (h264Packet == nullptr) {
        LOGE("fill_h264_packet_callback_ Get null packet_");
        return VIDEO_QUEUE_ABORT_ERR_CODE;
    }
    int bufferSize = (h264Packet)->size;
    uint8_t* outputData = (uint8_t *) ((h264Packet)->buffer);
    last_presentation_time_ms_ = h264Packet->timeMills;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    pkt.stream_index = st->index;
    int64_t cal_pts = last_presentation_time_ms_ / 1000.0f / av_q2d(video_stream_->time_base);
    int64_t pts = h264Packet->pts == PTS_PARAM_UN_SETTIED_FLAG ? cal_pts : h264Packet->pts;
    int64_t dts = h264Packet->dts == DTS_PARAM_UN_SETTIED_FLAG ? pts : h264Packet->dts == DTS_PARAM_NOT_A_NUM_FLAG ? AV_NOPTS_VALUE : h264Packet->dts;
    int nalu_type = (outputData[4] & 0x1F);
    if (nalu_type == H264_NALU_TYPE_SEQUENCE_PARAMETER_SET) {
        //我们这里要求sps和pps一块拼接起来构造成AVPacket传过来
        header_size_ = bufferSize;
        header_data_ = new uint8_t[header_size_];
        memcpy(header_data_, outputData, bufferSize);

        uint8_t* spsFrame = 0;
        uint8_t* ppsFrame = 0;

        int spsFrameLen = 0;
        int ppsFrameLen = 0;

        ParseH264SequenceHeader(header_data_, header_size_, &spsFrame, spsFrameLen,
                                &ppsFrame, ppsFrameLen);

        // Extradata contains PPS & SPS for AVCC format
        int extradata_len = 8 + spsFrameLen - 4 + 1 + 2 + ppsFrameLen - 4;
        c->extradata = (uint8_t*) av_mallocz(extradata_len);
        c->extradata_size = extradata_len;
        c->extradata[0] = 0x01;
        c->extradata[1] = spsFrame[4 + 1];
        c->extradata[2] = spsFrame[4 + 2];
        c->extradata[3] = spsFrame[4 + 3];
        c->extradata[4] = 0xFC | 3;
        c->extradata[5] = 0xE0 | 1;
        int tmp = spsFrameLen - 4;
        c->extradata[6] = (tmp >> 8) & 0x00ff;
        c->extradata[7] = tmp & 0x00ff;
        int i = 0;
        for (i = 0; i < tmp; i++)
            c->extradata[8 + i] = spsFrame[4 + i];
        c->extradata[8 + tmp] = 0x01;
        int tmp2 = ppsFrameLen - 4;
        c->extradata[8 + tmp + 1] = (tmp2 >> 8) & 0x00ff;
        c->extradata[8 + tmp + 2] = tmp2 & 0x00ff;
        for (i = 0; i < tmp2; i++)
            c->extradata[8 + tmp + 3 + i] = ppsFrame[4 + i];

        ret = avformat_write_header(oc, nullptr);
        if (ret < 0) {
            LOGE("Error occurred when opening output file: %s\n", av_err2str(ret));
        } else{
            write_header_success_ = true;
        }
    } else {
        if (nalu_type == H264_NALU_TYPE_IDR_PICTURE || nalu_type == H264_NALU_TYPE_SEI) {
            pkt.size = bufferSize;
            pkt.data = outputData;

            if(pkt.data[0] == 0x00 && pkt.data[1] == 0x00 &&
               pkt.data[2] == 0x00 && pkt.data[3] == 0x01){
                bufferSize -= 4;
                pkt.data[0] = ((bufferSize) >> 24) & 0x00ff;
                pkt.data[1] = ((bufferSize) >> 16) & 0x00ff;
                pkt.data[2] = ((bufferSize) >> 8) & 0x00ff;
                pkt.data[3] = ((bufferSize)) & 0x00ff;
            }

            pkt.pts = pts;
            pkt.dts = dts;
            pkt.flags = AV_PKT_FLAG_KEY;
            c->frame_number++;
        }
        else {
            pkt.size = bufferSize;
            pkt.data = outputData;

            if(pkt.data[0] == 0x00 && pkt.data[1] == 0x00 &&
               pkt.data[2] == 0x00 && pkt.data[3] == 0x01){
                bufferSize -= 4;
                pkt.data[0] = ((bufferSize ) >> 24) & 0x00ff;
                pkt.data[1] = ((bufferSize ) >> 16) & 0x00ff;
                pkt.data[2] = ((bufferSize ) >> 8) & 0x00ff;
                pkt.data[3] = ((bufferSize )) & 0x00ff;
            }

            pkt.pts = pts;
            pkt.dts = dts;
            pkt.flags = 0;
            c->frame_number++;
        }
        if (pkt.size) {
            ret = av_interleaved_write_frame(oc, &pkt);
            if (ret != 0) {
            }
        } else {
            ret = 0;
        }
    }
    av_free_packet(&pkt);
    delete h264Packet;
    return ret;
}

double H264Muxer::GetVideoStreamTimeInSecs() {
    return last_presentation_time_ms_ / 1000.0f;
}

uint32_t H264Muxer::FindStartCode(uint8_t *in_buffer, uint32_t in_ui32_buffer_size, uint32_t in_ui32_code,
                                  uint32_t &out_ui32_processed_bytes) {
    uint32_t ui32Code = in_ui32_code;

    const uint8_t * ptr = in_buffer;
    while (ptr < in_buffer + in_ui32_buffer_size) {
        ui32Code = *ptr++ + (ui32Code << 8);
        if (is_start_code(ui32Code))
            break;
    }

    out_ui32_processed_bytes = (uint32_t)(ptr - in_buffer);

    return ui32Code;
}

void
H264Muxer::ParseH264SequenceHeader(uint8_t *in_buffer, uint32_t in_ui32_size, uint8_t **in_sps_buffer, int &in_sps_size,
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

        if (ui32BufferSize < 1)
            break;

        uint8_t val = (*pBuffer & 0x1f);

        if (val == 5)
            idr = pps + ui32ProcessedBytes - 4;

        if (val == 7)
            sps = ui32ProcessedBytes;

        if (val == 8)
            pps = sps + ui32ProcessedBytes;

    } while (ui32BufferSize > 0);

    *in_sps_buffer = in_buffer + sps - 4;
    in_sps_size = pps - sps;

    *in_pps_buffer = in_buffer + pps - 4;
    in_pps_size = idr - pps + 4;
}

}
