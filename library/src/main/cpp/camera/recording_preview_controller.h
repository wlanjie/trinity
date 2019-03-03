#ifndef MV_RECORDING_PREVIEW_CONTROLLER_H
#define MV_RECORDING_PREVIEW_CONTROLLER_H

#include <unistd.h>
#include <pthread.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "egl_core/egl_core.h"
#include "hw_encoder/hw_encoder_adapter.h"
#include "recording_preview_renderer.h"
#include "soft_encoder/soft_encoder_adapter.h"

#define CAMERA_FACING_BACK                                        0
#define CAMERA_FACING_FRONT                                        1

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

class RecordingPreviewController {
public:
    RecordingPreviewController();

    virtual ~RecordingPreviewController();

    /** 1:准备EGL Context与EGLThread **/
    void PrepareEGLContext(ANativeWindow *window, JavaVM *g_jvm, jobject obj, int screenWidth,
                           int screenHeight, int cameraFacingId);

    /** 2:当Camera捕捉到新的一帧图像会调用 **/
    void NotifyFrameAvailable();

    /** 4:切换摄像头转向 **/
    void SwitchCameraFacing();

    /** 调整视频编码质量 **/
    void AdaptiveVideoQuality(int maxBitRate, int avgBitRate, int fps);

    void HotConfig(int bitRate, int fps, int gopSize);

    void HotConfigQuality(int maxBitrate, int avgBitrate, int fps);

    /** 5:重置绘制区域大小 **/
    void ResetRenderSize(int screenWidth, int screenHeight);

    /** 7:销毁EGLContext与EGLThread **/
    virtual void DestroyEGLContext();

private:
    virtual void BuildRenderInstance();

    virtual void ProcessVideoFrame(float position);

    void Draw();

    void ConfigCamera();

    void StartCameraPreview();

    void UpdateTexImage();

    void ReleaseCamera();

    static void *ThreadStartCallback(void *myself);

    void ProcessMessage();
protected:
    ANativeWindow *window_;
    JavaVM *vm_;
    jobject obj_;
    int screen_width_;
    int screen_height_;
    bool switch_camera_;
    int64_t start_time_;
    int facing_id_;
    int degress_;
    int texture_width_;
    int texture_height_;

    int camera_width_;
    int camera_height_;
    RecordingPreviewHandler *handler_;
    MessageQueue *queue_;
    pthread_t thread_id_;
    EGLCore *egl_core_;
    EGLSurface preview_surface_;
    RecordingPreviewRenderer *renderer_;
    bool thread_create_succeed_;
    bool encoding_;
    VideoEncoderAdapter *encoder_;

public:
    void StartEncoding(int width, int height, int videoBitRate, float frameRate,
                       bool useHardWareEncoding, int strategy);

    void StopEncoding();

    void CreateWindowSurface(ANativeWindow *window);

    void DestroyWindowSurface();

public:
    virtual bool Initialize();

    //销毁EGL资源并且调用Andorid销毁Camera
    virtual void Destroy();

    void CreatePreviewSurface();

    void DestroyPreviewSurface();

    void SwitchCamera();

    void StartRecording();

    void StopRecording();

    void RenderFrame();
};


class RecordingPreviewHandler : public Handler {
private:
    RecordingPreviewController *previewController;

public:
    RecordingPreviewHandler(RecordingPreviewController *previewController, MessageQueue *queue)
            :
            Handler(queue) {
        this->previewController = previewController;
    }

    void handleMessage(Message *msg) {
        int what = msg->getWhat();
        switch (what) {
            case MSG_EGL_THREAD_CREATE:
                previewController->Initialize();
                break;
            case MSG_EGL_CREATE_PREVIEW_SURFACE:
                previewController->CreatePreviewSurface();
                break;
            case MSG_SWITCH_CAMERA_FACING:
                previewController->SwitchCamera();
                break;
            case MSG_START_RECORDING:
                previewController->StartRecording();
                break;
            case MSG_STOP_RECORDING:
                previewController->StopRecording();
                break;
            case MSG_EGL_DESTROY_PREVIEW_SURFACE:
                previewController->DestroyPreviewSurface();
                break;
            case MSG_EGL_THREAD_EXIT:
                previewController->Destroy();
                break;
            case MSG_RENDER_FRAME:
                previewController->RenderFrame();
                break;
            default:
                break;
        }
    }
};

#endif // MV_RECORDING_PREVIEW_CONTROLLER_H
