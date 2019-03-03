#ifndef VIDEO_ENCODER_ADAPTER_H
#define VIDEO_ENCODER_ADAPTER_H

#include "CommonTools.h"
#include "opengl_media/render/video_gl_surface_render.h"
#include "egl_core/egl_core.h"
#include "live_common_packet_pool.h"

class VideoEncoderAdapter {
public:
    VideoEncoderAdapter();

    virtual ~VideoEncoderAdapter();

    virtual void init(int width, int height, int videoBitRate, float frameRate);

    virtual void createEncoder(EGLCore *eglCore, int inputTexId) = 0;

    virtual void encode() = 0;

    virtual void destroyEncoder() = 0;

    virtual void reConfigure(int maxBitRate, int avgBitRate, int fps) = 0;

    virtual void hotConfig(int maxBitrate, int avgBitrate, int fps) = 0;

    void resetFpsStartTimeIfNeed(int fps);

protected:
    int encodedFrameCount;
    int videoWidth;
    int videoHeight;
    int videoBitRate;
    float frameRate;
    LiveCommonPacketPool *packetPool;

    int64_t startTime;
    int64_t fpsChangeTime;

    VideoGLSurfaceRender *renderer;
    int texId;

    const float FLOAT_DELTA = 1e-4;

};

#endif // VIDEO_ENCODER_ADAPTER_H
