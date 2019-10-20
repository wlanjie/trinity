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

#ifndef TRINITY_SOFT_ENCODER_ADAPTER_H
#define TRINITY_SOFT_ENCODER_ADAPTER_H

#include <unistd.h>
#include "video_encoder_adapter.h"
#include "video_x264_encoder.h"
#include "egl_core.h"
#include "opengl.h"
#include "encode_render.h"

namespace trinity {

class SoftEncoderAdapter : public VideoEncoderAdapter {
 public:
    explicit SoftEncoderAdapter(GLfloat* vertex_coordinate = nullptr, GLfloat* texture_coordinate = nullptr);

    virtual ~SoftEncoderAdapter();

    void CreateEncoder(EGLCore *eglCore, int inputTexId);

    void Encode(int timeMills = -1);

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
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    GLuint fbo_;
    GLuint output_texture_id_;
    EGLCore *egl_core_;
    EGLSurface copy_texture_surface_;
    enum DownloadThreadMessage {
        MSG_NONE = 0, MSG_WINDOW_SET, MSG_RENDER_LOOP_EXIT
    };
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    enum DownloadThreadMessage msg_;
    pthread_t image_download_thread_;
    EncodeRender* encode_render_;
    int pixel_size_;
    VideoX264Encoder *encoder_;
    pthread_t x264_encoder_thread_;
    OpenGL *renderer_;
    int time_mills_;
};

}  // namespace trinity

#endif  // TRINITY_SOFT_ENCODER_ADAPTER_H
