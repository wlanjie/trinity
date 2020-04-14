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

#include "video_x264_encoder.h"
#include "android_xlog.h"

namespace trinity {

VideoX264Encoder::VideoX264Encoder(int strategy) {
    strategy_ = strategy;
    codec_context_ = nullptr;
    codec_ = nullptr;
    picture_buf_ = nullptr;
    frame_ = nullptr;
    yuy2_picture_buf_ = nullptr;
    video_yuy2_frame_ = nullptr;
    packet_pool_ = nullptr;
    sps_unwrite_flag_ = false;
}

VideoX264Encoder::~VideoX264Encoder() {
}

int VideoX264Encoder::Init(int width, int height, int videoBitRate, float frameRate,
                           PacketPool *packetPool) {
    LOGI("enter %s", __func__);
    if (AllocVideoStream(width, height, videoBitRate, frameRate) < 0) {
        LOGE("alloc Video Stream Failed... \n");
        return -1;
    }
    AllocAVFrame();
    frame_->width = width;
    frame_->height = height;
    frame_->format = AV_PIX_FMT_YUV420P;
    packet_pool_ = packetPool;
    sps_unwrite_flag_ = true;
    LOGI("leave %s", __func__);
    return 0;
}

int VideoX264Encoder::Encode(VideoPacket *yuy2VideoPacket) {
    if (nullptr == yuy2_picture_buf_) {
        return -1;
    }
    memcpy(yuy2_picture_buf_, yuy2VideoPacket->buffer, static_cast<size_t>(yuy2VideoPacket->size));
    frame_->data[0] = yuy2VideoPacket->buffer;
    frame_->data[1] = yuy2VideoPacket->buffer + frame_->width * frame_->height;
    frame_->data[2] = yuy2VideoPacket->buffer + frame_->width * frame_->height +
                      frame_->width * frame_->height / 4;
    int time_mills = yuy2VideoPacket->timeMills;
    AVRational time_base = {1, 1000};
    int64_t pts = (int64_t) (time_mills / 1000.0f / av_q2d(time_base));
    frame_->pts = pts;
    int ret = avcodec_send_frame(codec_context_, frame_);
    if (ret < 0) {
        LOGE("x264 send frame error: %s", av_err2str(ret));
        return ret;
    }
    while (true) {
        AVPacket pkt = {0};
        av_init_packet(&pkt);
        ret = avcodec_receive_packet(codec_context_, &pkt);
        if (ret < 0) {
            av_packet_unref(&pkt);
            break;
        }
        int presentationTimeMills = static_cast<int>(pkt.pts * av_q2d(time_base) * 1000.0f);
        int nalu_type = (pkt.data[4] & 0x1F);
        bool isKeyFrame = false;
        uint8_t *frameBuffer;
        int frameBufferSize = 0;
        const char bytesHeader[] = "\x00\x00\x00\x01";
        // string literals have implicit trailing '\0'
        size_t headerLength = 4;
        if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET == nalu_type) {
            // 说明是关键帧
            isKeyFrame = true;
            // 分离出sps pps
            std::vector<NALUnit *> *units = new std::vector<NALUnit *>();
            parseH264SpsPps(pkt.data, static_cast<uint32_t>(pkt.size), units);
            if (sps_unwrite_flag_) {
                NALUnit *sps_unit = units->at(0);
                uint8_t *sps_frame = sps_unit->naluBody;
                int sps_frame_len = sps_unit->naluSize;
                NALUnit *pps_unit = units->at(1);
                uint8_t *pps_frame = pps_unit->naluBody;
                int pps_frame_len = pps_unit->naluSize;
                // 将sps、pps写出去
                auto meta_buffer_size = headerLength + sps_frame_len + headerLength + pps_frame_len;
                uint8_t *meta_buffer = new uint8_t[meta_buffer_size];
                memcpy(meta_buffer, bytesHeader, headerLength);
                memcpy(meta_buffer + headerLength, sps_frame, static_cast<size_t>(sps_frame_len));
                memcpy(meta_buffer + headerLength + sps_frame_len, bytesHeader, headerLength);
                memcpy(meta_buffer + headerLength + sps_frame_len + headerLength, pps_frame,
                        static_cast<size_t>(pps_frame_len));
                PushToQueue(meta_buffer, static_cast<int>(meta_buffer_size), presentationTimeMills, pkt.pts, pkt.dts);
                sps_unwrite_flag_ = false;
            }
            std::vector<NALUnit *>::iterator i;
            for (i = units->begin(); i != units->end(); ++i) {
                NALUnit *unit = *i;
                nalu_type = unit->naluType;
                if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET != nalu_type &&
                    H264_NALU_TYPE_PICTURE_PARAMETER_SET != nalu_type) {
                    int idrFrameLen = unit->naluSize;
                    frameBufferSize += headerLength;
                    frameBufferSize += idrFrameLen;
                }
            }
            frameBuffer = new uint8_t[frameBufferSize];
            int frameBufferCursor = 0;
            for (i = units->begin(); i != units->end(); ++i) {
                NALUnit *unit = *i;
                nalu_type = unit->naluType;
                if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET != nalu_type &&
                    H264_NALU_TYPE_PICTURE_PARAMETER_SET != nalu_type) {
                    uint8_t *idrFrame = unit->naluBody;
                    int idrFrameLen = unit->naluSize;
                    //将关键帧分离出来
                    memcpy(frameBuffer + frameBufferCursor, bytesHeader, headerLength);
                    frameBufferCursor += headerLength;
                    memcpy(frameBuffer + frameBufferCursor, idrFrame,
                           static_cast<size_t>(idrFrameLen));
                    frameBufferCursor += idrFrameLen;
                    frameBuffer[frameBufferCursor - idrFrameLen - headerLength] =
                            static_cast<uint8_t>(((idrFrameLen) >> 24) & 0x00ff);
                    frameBuffer[frameBufferCursor - idrFrameLen - headerLength + 1] =
                            static_cast<uint8_t>(((idrFrameLen) >> 16) & 0x00ff);
                    frameBuffer[frameBufferCursor - idrFrameLen - headerLength + 2] =
                            static_cast<uint8_t>(((idrFrameLen) >> 8) & 0x00ff);
                    frameBuffer[frameBufferCursor - idrFrameLen - headerLength + 3] =
                            static_cast<uint8_t>(((idrFrameLen)) & 0x00ff);
                }
                delete unit;
            }
            delete units;
        } else {
            //说明是非关键帧, 从Packet里面分离出来
            isKeyFrame = false;
            std::vector<NALUnit *> *units = new std::vector<NALUnit *>();
            parseH264SpsPps(pkt.data, static_cast<uint32_t>(pkt.size), units);
            std::vector<NALUnit *>::iterator i;
            for (i = units->begin(); i != units->end(); ++i) {
                NALUnit *unit = *i;
                int nonIDRFrameLen = unit->naluSize;
                frameBufferSize += headerLength;
                frameBufferSize += nonIDRFrameLen;
            }
            frameBuffer = new uint8_t[frameBufferSize];
            int frameBufferCursor = 0;
            for (i = units->begin(); i != units->end(); ++i) {
                NALUnit *unit = *i;
                uint8_t *nonIDRFrame = unit->naluBody;
                int nonIDRFrameLen = unit->naluSize;
                memcpy(frameBuffer + frameBufferCursor, bytesHeader, headerLength);
                frameBufferCursor += headerLength;
                memcpy(frameBuffer + frameBufferCursor, nonIDRFrame,
                       static_cast<size_t>(nonIDRFrameLen));
                frameBufferCursor += nonIDRFrameLen;
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength] =
                        static_cast<uint8_t>(((nonIDRFrameLen) >> 24) & 0x00ff);
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 1] =
                        static_cast<uint8_t>(((nonIDRFrameLen) >> 16) & 0x00ff);
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 2] =
                        static_cast<uint8_t>(((nonIDRFrameLen) >> 8) & 0x00ff);
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 3] =
                        static_cast<uint8_t>(((nonIDRFrameLen)) & 0x00ff);
                delete unit;
            }
            delete units;
        }
        PushToQueue(frameBuffer, frameBufferSize, presentationTimeMills, pkt.pts, pkt.dts);
    }
    return 0;
}

void
VideoX264Encoder::PushToQueue(uint8_t *buffer, int size, int timeMills, int64_t pts, int64_t dts) {
    auto *h264Packet = new VideoPacket();
    h264Packet->buffer = buffer;
    h264Packet->size = size;
    h264Packet->timeMills = timeMills;
    // h264Packet->pts = pts;
    // h264Packet->dts = dts;
    packet_pool_->PushRecordingVideoPacketToQueue(h264Packet);
}

int VideoX264Encoder::Destroy() {
    // Clean
    if (nullptr != codec_context_) {
        avcodec_close(codec_context_);
        codec_context_ = nullptr;
    }
    if (frame_ != nullptr) {
        av_free(frame_);
        frame_ = nullptr;
    }
    if (picture_buf_ != nullptr) {
        av_free(picture_buf_);
        picture_buf_ = nullptr;
    }
    // TODO release yuy2_picture_buf_
    return 0;
}

void VideoX264Encoder::ReConfigure(int bitRate) {
    if (strategy_ == 1) {
        // 修正比特率
        int bitRateToSet = bitRate - delta_;
        codec_context_->rc_max_rate = bitRateToSet;
        codec_context_->rc_min_rate = bitRateToSet;
        codec_context_->bit_rate = bitRateToSet;
        codec_context_->rc_buffer_size = bitRate * 3;
    } else {
        codec_context_->rc_max_rate = bitRate + delta_;
        codec_context_->rc_min_rate = bitRate - delta_;
        codec_context_->rc_buffer_size = bitRate * 2;
        codec_context_->bit_rate = bitRate;
    }
    LOGI("VideoX264Encoder::ReConfigure strategy_:%d, max_rate:%d, min_rate:%d, bit_rate:%d",
         strategy_,
         codec_context_->rc_max_rate, codec_context_->rc_min_rate, codec_context_->bit_rate);
}

int VideoX264Encoder::AllocVideoStream(int width, int height, int videoBitRate, float frameRate) {
    codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (nullptr == codec_) {
        LOGE("Can not find encoder_! \n");
        return -1;
    }
    codec_context_ = avcodec_alloc_context3(codec_);
    codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context_->width = width;
    codec_context_->height = height;
    codec_context_->time_base.num = 1;
    codec_context_->time_base.den = static_cast<int>(frameRate);
    codec_context_->gop_size = static_cast<int>(frameRate);
    codec_context_->max_b_frames = 0;

    ReConfigure(videoBitRate);
    if (strategy_ == 1) {
        // cbr模式设置
        codec_context_->rc_initial_buffer_occupancy = codec_context_->rc_buffer_size * 3 / 4;
        codec_context_->rc_buffer_aggressivity = static_cast<float>(1.0F);
        codec_context_->rc_initial_cplx = 0.5;
        codec_context_->qcompress = 0;
    } else {
        // vbr模式
        /**  -qscale q  使用固定的视频量化标度(VBR)  以<q>质量为基础的VBR，取值0.01-255，约小质量越好，即qscale 4和-qscale 6，4的质量比6高 。
             * 					此参数使用次数较多，实际使用时发现，qscale是种固定量化因子，设置qscale之后，前面设置的-b好像就无效了，而是自动调整了比特率。
             *	 -qmin q 最小视频量化标度(VBR) 设定最小质量，与-qmax（设定最大质量）共用
             *	 -qmax q 最大视频量化标度(VBR) 使用了该参数，就可以不使用qscale参数  **/
        codec_context_->flags |= CODEC_FLAG_QSCALE;
        codec_context_->i_quant_factor = 0.8;
        codec_context_->qmin = 10;
        codec_context_->qmax = 30;
    }

    // H.264
    // 新增语句，设置为编码延迟
    av_opt_set(codec_context_->priv_data, "preset", "ultrafast", 0);
    // 实时编码关键看这句，上面那条无所谓
    av_opt_set(codec_context_->priv_data, "tune", "zerolatency", 0);
    av_opt_set(codec_context_->priv_data, "profile", "main", 0);
    // av_opt_set(codec_context_->priv_data, "crf", "20", AV_OPT_SEARCH_CHILDREN);
    // av_opt_set(codec_context_->priv_data, "crf", "30", AV_OPT_SEARCH_CHILDREN);
    if (avcodec_open2(codec_context_, codec_, nullptr) < 0) {
        LOGE("Failed to open encoder_! \n");
        return -1;
    }
    return 0;
}

void VideoX264Encoder::AllocAVFrame() {
    // target yuv420p buffer_
    frame_ = av_frame_alloc();
    int yuv_420p_size = av_image_get_buffer_size(codec_context_->pix_fmt, codec_context_->width,
                                                 codec_context_->height, 1);
    picture_buf_ = reinterpret_cast<uint8_t *>(av_malloc(static_cast<size_t>(yuv_420p_size)));
    av_image_fill_arrays(frame_->data, frame_->linesize, picture_buf_, codec_context_->pix_fmt,
            codec_context_->width, codec_context_->height, 1);
    // origin yuy2 buffer_
    video_yuy2_frame_ = av_frame_alloc();
    int yuy_size = av_image_get_buffer_size(ORIGIN_COLOR_FORMAT, codec_context_->width,
                                            codec_context_->height, 1);
    yuy2_picture_buf_ = reinterpret_cast<uint8_t *>(av_malloc(static_cast<size_t>(yuy_size)));
    av_image_fill_arrays(video_yuy2_frame_->data, video_yuy2_frame_->linesize, yuy2_picture_buf_,
            ORIGIN_COLOR_FORMAT, codec_context_->width, codec_context_->height, 1);
}

}  // namespace trinity
