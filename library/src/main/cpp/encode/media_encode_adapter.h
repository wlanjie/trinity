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

//
// Created by wlanjie on 2019/4/16.
//

#ifndef TRINITY_MEDIA_ENCODE_ADAPTER_H
#define TRINITY_MEDIA_ENCODE_ADAPTER_H

#include <jni.h>
#include "video_encoder_adapter.h"
#include "handler.h"
#include "video_packet_queue.h"
#include "opengl.h"

namespace trinity {

typedef enum {
    FRAME_AVAILABLE
} MediaEncoderType;

class MediaEncodeAdapter : public VideoEncoderAdapter, public Handler {
 public:
    MediaEncodeAdapter(JavaVM* vm, jobject object);
    virtual ~MediaEncodeAdapter();

    virtual void CreateEncoder(EGLCore* core);

    virtual void DestroyEncoder();

    virtual void Encode(uint64_t time, int texture_id = 0);

    void DrainEncodeData();

 private:
    bool encoding_;
    bool sps_write_flag_;
    JavaVM* vm_;
    jobject object_;
    EGLCore* core_;
    MessageQueue* queue_;
    pthread_t encoder_thread_;
    EGLSurface encoder_surface_;
    ANativeWindow* encoder_window_;
    jbyteArray output_buffer_;
    long start_encode_time_;
    OpenGL* render_;

 private:
    static void* MessageQueueThread(void* args);
    virtual void HandleMessage(Message* msg);
    void EncodeLoop();
    void CreateMediaEncoder(JNIEnv* env);
    void DestroyMediaEncoder(JNIEnv* env);
};

}  // namespace trinity

#endif  // TRINITY_MEDIA_ENCODE_ADAPTER_H
