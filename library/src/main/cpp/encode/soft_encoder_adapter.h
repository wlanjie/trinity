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
#include "handler.h"

namespace trinity {

class SoftEncoderAdapter : public VideoEncoderAdapter, public Handler {
 public:
    explicit SoftEncoderAdapter(GLfloat* vertex_coordinate = nullptr, GLfloat* texture_coordinate = nullptr);

    virtual ~SoftEncoderAdapter();

    void CreateEncoder(EGLCore *eglCore);

    void Encode(uint64_t time, int texture_id = 0);

    void DestroyEncoder();

 private:
    void StartEncode();
    static void* StartEncodeThread(void* args);
    static void* MessageQueueThread(void* args);
    virtual void HandleMessage(Message* msg);
    void EncodeLoop();
    void CreateX264Encoder(EGLCore* core);
    void EncodeFrame(int texture_id, int time);
    void DestroyX264Encoder();
    bool Initialize();
    void EncodeTexture(GLuint texture_id, int time);
 private:
    bool encoding_;
    MessageQueue* queue_;
    EGLCore* core_;
    EGLSurface encoder_surface_;
    pthread_t encoder_thread_;
    VideoPacketQueue *yuy_packet_pool_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    GLuint fbo_;
    GLuint output_texture_id_;
    EncodeRender* encode_render_;
    int pixel_size_;
    VideoX264Encoder *encoder_;
    pthread_t x264_encoder_thread_;
    OpenGL *renderer_;
    pthread_mutex_t packet_mutex_;
    pthread_cond_t packet_cond_;
};

}  // namespace trinity

#endif  // TRINITY_SOFT_ENCODER_ADAPTER_H
