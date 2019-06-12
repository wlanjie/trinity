//
// Created by wlanjie on 2019/4/15.
//

#ifndef TRINITY_VIDEO_ENCODER_ADAPTER_H
#define TRINITY_VIDEO_ENCODER_ADAPTER_H

#include "egl_core.h"
#include "packet_pool.h"

namespace trinity {

class VideoEncoderAdapter {
public:
    VideoEncoderAdapter();
    ~VideoEncoderAdapter();

    virtual void Init(int width, int height, int video_bit_rate, int frame_rate);

    virtual void CreateEncoder(EGLCore* core, int texture_id) = 0;

    virtual void Encode() = 0;

    virtual void DestroyEncoder() = 0;

    void ResetFpsStartTimeIfNeed(int fps);
protected:
    int encode_frame_count_;
    int video_width_;
    int video_height_;
    int video_bit_rate_;
    int frame_rate_;
    int64_t fps_change_time_;
    int texture_id_;
    int64_t start_time_;
    const float FLOAT_DELTA = 1e-4;
    PacketPool* packet_pool_;
};

}

#endif //TRINITY_VIDEO_ENCODER_ADAPTER_H
