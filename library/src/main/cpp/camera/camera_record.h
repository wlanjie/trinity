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
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_CAMERA_RECORD_H
#define TRINITY_CAMERA_RECORD_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "egl_core.h"
#include "handler.h"
#include "frame_buffer.h"
#include "video_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "soft_encoder_adapter.h"

namespace trinity {

enum RenderThreadMessage {
    MSG_RENDER_FRAME = 0,
    MSG_EGL_THREAD_CREATE,
    MSG_EGL_CREATE_PREVIEW_SURFACE,
    MSG_SWITCH_CAMERA_FACING,
    MSG_SET_FRAME,
    MSG_START_RECORDING,
    MSG_STOP_RECORDING,
    MSG_EGL_DESTROY_PREVIEW_SURFACE,
    MSG_EGL_THREAD_EXIT
};

class CameraRecordHandler;

class CameraRecord {
 public:
    explicit CameraRecord(JNIEnv* env);
    virtual ~CameraRecord();

    void PrepareEGLContext(jobject object, jobject surface,
            int screen_width, int screen_height);

    void NotifyFrameAvailable();

    /**
     * 设置录制速度
     * @param speed 0.25f, 0.5f, 1.0f, 2.0f, 4.0f
     */
    void SetSpeed(float speed);

    void SetRenderType(int type);

    void SetFrame(int frame);

    void SwitchCameraFacing();

    void ResetRenderSize(int screen_width, int screen_height);

    virtual void DestroyEGLContext();

    virtual bool Initialize();

    virtual void Destroy();

    void CreatePreviewSurface();

    void DestroyPreviewSurface();

    void CreateWindowSurface(ANativeWindow *window);

    void DestroyWindowSurface();

    void SwitchCamera();

    void StartEncoding(const char* path,
            int width,
            int height,
            int video_bit_rate,
            int frame_rate,
            bool use_hard_encode,
            int audio_sample_rate,
            int audio_channel,
            int audio_bit_rate);

    void StartRecording();

    void StopEncoding();

    void StopRecording();

    void RenderFrame();

    void SetFrameType(int frame);

 private:
    void Draw();

    void ConfigCamera();

    void StartCameraPreview();

    void UpdateTexImage();

    void onSurfaceCreated();

    int OnDrawFrame(int texture_id, int width, int height);

    void onSurfaceDestroy();

    void GetTextureMatrix();

    void ReleaseCamera();

    static void *ThreadStartCallback(void *myself);

    void ProcessMessage();

 private:
    ANativeWindow *window_;
    JNIEnv* env_;
    JavaVM *vm_;
    jobject obj_;
    int screen_width_;
    int screen_height_;
    int camera_width_;
    int camera_height_;
    bool camera_size_change_;

    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    EGLCore *egl_core_;
    EGLSurface preview_surface_;
    FrameBuffer* frame_buffer_;
    OpenGL* render_screen_;
    CameraRecordHandler* handler_;
    MessageQueue* queue_;
    pthread_t thread_id_;
    GLuint oes_texture_id_;
    GLfloat* texture_matrix_;
    VideoEncoderAdapter* encoder_;
    bool encoding_;
    VideoConsumerThread* packet_thread_;
    int64_t start_time_;
    float speed_;
    int render_type_;
};

class CameraRecordHandler : public Handler {
 public:
    CameraRecordHandler(CameraRecord* record,
            MessageQueue* queue) : Handler(queue) {
        record_ = record;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        switch (what) {
            case MSG_EGL_THREAD_CREATE:
                record_->Initialize();
                break;
            case MSG_EGL_CREATE_PREVIEW_SURFACE:
                record_->CreatePreviewSurface();
                break;
            case MSG_SWITCH_CAMERA_FACING:
                record_->SwitchCamera();
                break;
            case MSG_START_RECORDING:
                record_->StartRecording();
                break;
            case MSG_STOP_RECORDING:
                record_->StopRecording();
                break;
            case MSG_EGL_DESTROY_PREVIEW_SURFACE:
                record_->DestroyPreviewSurface();
                break;
            case MSG_EGL_THREAD_EXIT:
                record_->Destroy();
                break;
            case MSG_RENDER_FRAME:
                record_->RenderFrame();
                break;
            case MSG_SET_FRAME:
                record_->SetFrameType(msg->GetArg1());
                break;
            default:
                break;
        }
    }

 private:
    CameraRecord* record_;
};

}  // namespace trinity

#endif  // TRINITY_CAMERA_RECORD_H
