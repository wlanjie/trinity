//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_CAMERA_RECORD_H
#define TRINITY_CAMERA_RECORD_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <opengl/render_screen.h>
#include "egl_core.h"
#include "handler.h"
#include "frame_buffer.h"
#include "render_screen.h"
#include "video_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "soft_encoder_adapter.h"

#define CAMERA_FACING_BACK                                        0
#define CAMERA_FACING_FRONT                                       1

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
    CameraRecord();
    virtual ~CameraRecord();

    void PrepareEGLContext(ANativeWindow* window, JavaVM* vm, jobject object, int screen_width, int screen_height, int camera_id);

    void NotifyFrameAvailable();

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
    virtual void ProcessVideoFrame(float position);

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
    JavaVM *vm_;
    jobject obj_;
    int screen_width_;
    int screen_height_;
    bool switch_camera_;
    int facing_id_;
    int camera_width_;
    int camera_height_;

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
};

class CameraRecordHandler : public Handler {
public:
    CameraRecordHandler(CameraRecord* record, MessageQueue* queue) : Handler(queue) {
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

}

#endif //TRINITY_CAMERA_RECORD_H
