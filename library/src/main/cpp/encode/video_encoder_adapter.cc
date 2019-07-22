//
// Created by wlanjie on 2019/4/15.
//

#include <math.h>
#include "video_encoder_adapter.h"
#include "tools.h"

namespace trinity {

VideoEncoderAdapter::VideoEncoderAdapter() {
    texture_id_ = 0;
    fps_change_time_ = 0;
    start_time_ = 0;
}

VideoEncoderAdapter::~VideoEncoderAdapter() {

}

void VideoEncoderAdapter::Init(int width, int height, int video_bit_rate, int frame_rate) {
    packet_pool_ = PacketPool::GetInstance();
    video_width_ = width;
    video_height_ = height;
    video_bit_rate_ = video_bit_rate;
    frame_rate_ = frame_rate;
    encode_frame_count_ = 0;
}

void VideoEncoderAdapter::ResetFpsStartTimeIfNeed(int fps) {
    if (fabs(fps - frame_rate_) > FLOAT_DELTA) {
        frame_rate_ = fps;
        encode_frame_count_ = 0;
        fps_change_time_ = getCurrentTime();
    }
}

}