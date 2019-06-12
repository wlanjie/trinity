#ifndef TRINITY_SOFT_ENCODER_ADAPTER_H
#define TRINITY_SOFT_ENCODER_ADAPTER_H

#include <unistd.h>
#include "video_encoder_adapter.h"
#include "video_x264_encoder.h"
#include "host_gpu_copier.h"
#include "egl_core.h"
#include "opengl.h"
#include "encode_render.h"

#define PIXEL_BYTE_SIZE 2

namespace trinity {

class SoftEncoderAdapter : public VideoEncoderAdapter {
public:
    SoftEncoderAdapter(int strategy);

    virtual ~SoftEncoderAdapter();

    void CreateEncoder(EGLCore *eglCore, int inputTexId);

    void Encode();

    void renderLoop();

    void startEncode();

    void DestroyEncoder();

    void ReConfigure(int maxBitRate, int avgBitRate, int fps);

    void HotConfig(int maxBitrate, int avgBitrate, int fps);

private:
    static void *StartEncodeThread(void *ptr);

    static void *StartDownloadThread(void *ptr);

    bool Initialize();

    void LoadTexture();

    void SignalPreviewThread();

    void Destroy();

private:
    VideoPacketQueue *yuy_packet_pool_;
    /** 这是创建RenderThread的context, 要共享给我们这个EGLContext线程 **/
    EGLContext load_texture_context_;
    GLuint fbo_;
    GLuint output_texture_id_;
    EGLCore *egl_core_;
    EGLSurface copy_texture_surface_;
    enum DownloadThreadMessage {
        MSG_NONE = 0, MSG_WINDOW_SET, MSG_RENDER_LOOP_EXIT
    };
    pthread_mutex_t preview_thread_lock_;
    pthread_cond_t preview_thread_condition_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    enum DownloadThreadMessage msg_;
    pthread_t image_download_thread_;
    int strategy_ = 0;
    EncodeRender* encode_render_;
    // TODO delete
    HostGPUCopier *host_gpu_copier_;
    int pixel_size_;
    VideoX264Encoder *encoder_;
    pthread_t x264_encoder_thread_;
    OpenGL *renderer_;
};

}
#endif // TRINITY_SOFT_ENCODER_ADAPTER_H
