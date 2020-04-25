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
#include <trinity.h>
#include "egl_core.h"
#include "handler.h"
#include "frame_buffer.h"
#include "video_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "soft_encoder_adapter.h"
#include "image_process.h"
#include "buffer_pool.h"
#include "face_detection.h"

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

class CameraRecord : public Handler, public FaceDetection {
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

    void CreateWindowSurface(ANativeWindow *window);

    void DestroyWindowSurface();

    void StartEncoding(const char* path,
            int width,
            int height,
            int video_bit_rate,
            int frame_rate,
            bool use_hard_encode,
            int audio_sample_rate,
            int audio_channel,
            int audio_bit_rate);

    void StopEncoding();

    int AddFilter(const char* config_path);

    void UpdateFilter(const char* config_path, int start_time, int end_time, int action_id);

    void DeleteFilter(int action_id);

    int AddAction(const char* config_path);

    void UpdateActionTime(int start_time, int end_time, int action_id);

    void UpdateActionParam(int action_id, const char* effect_name, const char* param_name, float value);

    void DeleteAction(int action_id);

    virtual void FaceDetector(std::vector<FaceDetectionReport*>& face_detection);
 private:
    void CreateEncode(bool media_codec_encode);

    virtual bool Initialize();

    virtual void Destroy();

    void CreatePreviewSurface();

    void DestroyPreviewSurface();

    void SwitchCamera();

    void StartRecording();

    void StopRecording();

    void RenderFrame();

    void SetFrameType(int frame);

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

    void FPS();

    void OnAddFilter(char* config_path, int action_id);

    void OnUpdateFilter(char* config_path, int action_id,
            int start_time, int end_time);

    void OnDeleteFilter(int action_id);

    void OnAddAction(char* config_path, int action_id);

    void OnUpdateActionTime(int start_time, int end_time, int action_id);

    void OnUpdateActionParam(EffectParam* param);

    void OnDeleteAction(int action_id);

    int GetCameraFacing();

    void GetFaceDetectionReports(std::vector<FaceDetectionReport*>& face_detection_reports);

    virtual void HandleMessage(Message* msg);
 private:
    ANativeWindow *window_;
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
    MessageQueue* queue_;
    pthread_t thread_id_;
    GLuint oes_texture_id_;
    GLfloat* texture_matrix_;
    VideoEncoderAdapter* encoder_;
    bool encoding_;
    VideoConsumerThread* packet_thread_;
    float speed_;
    int frame_type_;
    int frame_count_;
    int64_t pre_fps_count_time_;
    float fps_;
    bool start_recording;
    int current_action_id_;
    ImageProcess* image_process_;
    int64_t render_time_;
    int64_t encode_time_;
    int camera_facing_id_;
    BufferPool* message_pool_;
    bool media_codec_encode_;
    int video_width_;
    int video_height_;
    int frame_rate_;
    int video_bit_rate_;
};

}  // namespace trinity

#endif  // TRINITY_CAMERA_RECORD_H
