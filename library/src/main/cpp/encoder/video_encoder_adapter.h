#ifndef VIDEO_ENCODER_ADAPTER_H
#define VIDEO_ENCODER_ADAPTER_H

#include "common_tools.h"
#include "opengl_media/render/video_gl_surface_render.h"
#include "egl_core/egl_core.h"
#include "live_common_packet_pool.h"

class VideoEncoderAdapter {
public:
    VideoEncoderAdapter();

    virtual ~VideoEncoderAdapter();

    virtual void init(int width, int height, int videoBitRate, float frameRate);

    virtual void CreateEncoder(EGLCore *eglCore, int inputTexId) = 0;

    virtual void Encode() = 0;

    virtual void DestroyEncoder() = 0;

    virtual void ReConfigure(int maxBitRate, int avgBitRate, int fps) = 0;

    virtual void HotConfig(int maxBitrate, int avgBitrate, int fps) = 0;

    void ResetFpsStartTimeIfNeed(int fps);

protected:
    int encoded_frame_count_;
    int video_width_;
    int video_height_;
    int video_bit_rate_;
    float frameRate;
    LiveCommonPacketPool *packet_pool_;
    int64_t start_time_;
    int64_t fps_change_time_;
    VideoGLSurfaceRender *renderer_;
    int texture_id_;
    const float FLOAT_DELTA = 1e-4;

};

#endif // VIDEO_ENCODER_ADAPTER_H
