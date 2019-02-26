//
// Created by wlanjie on 2019/2/21.
//

#ifndef TRINITY_PREVIEW_CONTROLLER_H
#define TRINITY_PREVIEW_CONTROLLER_H

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "../common/egl/egl_core.h"
#include "../common/message_queue/handler.h"
#include "preview_renderer.h"

#define CAMERA_FACING_BACK                                        0
#define CAMERA_FACING_FRONT                                       1

namespace trinity {

enum RenderThreadMessage {
    MSG_RENDER_FRAME = 0,
    MSG_EGL_THREAD_CREATE,
    MSG_EGL_CREATE_PREVIEW_SURFACE,
    MSG_SWITCH_CAMERA_FACING,
    MSG_START_RECORDING,
    MSG_STOP_RECORDING,
    MSG_EGL_DESTROY_PREVIEW_SURFACE,
    MSG_EGL_THREAD_EXIT
};

class RecordingPreviewHandler;

class PreviewController {
public:
    PreviewController();
    virtual ~PreviewController();

    // 准备EGL Context与EGL Thread
    void PrepareEGLContext(ANativeWindow* window, JavaVM* vm, jobject obj, int screen_width, int screen_height, int camera_facing_id);

    // 当camera捕捉到新的一帧图像会调用
    void NotifyFrameAvailable();

    // 切换前后摄像头
    void SwitchCameraFacing();

    // 调整视频编码质量
    void AdaptiveVideoQuality(int max_bit_rate, int avg_bit_rate, int fps);

    // 调整绘制区域大小
    void ResetRenderSize(int screen_width, int screen_height);

    // 销毁EGLContext与EGLThread
    virtual void DestroyEGLContext();

protected:
    ANativeWindow* window_;
    JavaVM* vm_;
    jobject object_;
    int screen_width_;
    int screen_height_;

    bool is_switching_camera_;
    int64_t start_time_;

    int facing_id_;
    int degress_;
    int texture_width_;
    int texture_height_;

    int camera_width_;
    int camera_height_;

    RecordingPreviewHandler* handler;
    MessageQueue* queue_;
    pthread_t thread_id_;
protected:
    static void* ThreadStartCallback(void* my_self);
    void ProcessMessage();

    EGLCore* egl_core_;
    EGLSurface preview_surface_;
    // render
    PreviewRenderer* renderer_;

    virtual void BuildRenderInstance();

    virtual void ProcessVideoFrame(float position);

    void Draw();

    void ConfigCamera();

    void StartCameraPreview();

    void UpdateTextureImage();

    void ReleaseCamera();

protected:
    bool is_thread_create_succeed_;
    bool is_encoding_;
    // encode

public:
    void StartEncoding(int width, int height, int video_bit_rate, float frame_rate, bool use_hard_ware_encoding, int strategy);
    void StopEncoding();
    void CreateWindowSurface(ANativeWindow* window);
    void DestroyWindowSurface();

public:
    virtual bool Initialize();
    virtual void Destroy();
    void CreatePreviewSurface();
    void DestroyPreviewSurface();
    void SwitchCamera();
    void StartRecording();
    void StopRecording();
    void RenderFrame();
};

class RecordingPreviewHandler : public Handler {
public:
    RecordingPreviewHandler(PreviewController* preview_controller, MessageQueue* queue) : Handler(queue) {
        preview_controller_ = preview_controller;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        switch (what) {
            case MSG_EGL_THREAD_CREATE:
                preview_controller_->Initialize();
                break;

            case MSG_EGL_CREATE_PREVIEW_SURFACE:
                preview_controller_->CreatePreviewSurface();
                break;

            case MSG_SWITCH_CAMERA_FACING:
                preview_controller_->SwitchCamera();
                break;

            case MSG_START_RECORDING:
                preview_controller_->StartRecording();
                break;

            case MSG_STOP_RECORDING:
                preview_controller_->StopRecording();
                break;

            case MSG_EGL_DESTROY_PREVIEW_SURFACE:
                preview_controller_->DestroyPreviewSurface();

            case MSG_EGL_THREAD_EXIT:
                preview_controller_->Destroy();
                break;

            case MSG_RENDER_FRAME:
                preview_controller_->RenderFrame();
                break;

            default:
                break;
        }
    }
private:
    PreviewController* preview_controller_;
};

}
#endif //TRINITY_PREVIEW_CONTROLLER_H
