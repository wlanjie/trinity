#include "recording_preview_controller.h"

#define LOG_TAG "MVRecordingPreviewController"

RecordingPreviewController::RecordingPreviewController() {
    LOGI("RecordingPreviewController instance_ created");
    facing_id_ = CAMERA_FACING_FRONT;
    start_time_ = -1;
    egl_core_ = NULL;
    window_ = NULL;
    thread_create_succeed_ = false;
    encoder_ = NULL;
    preview_surface_ = EGL_NO_SURFACE;
    encoding_ = false;
    queue_ = new MessageQueue("RecordingPreviewController message queue_");
    handler_ = new RecordingPreviewHandler(this, queue_);
}

RecordingPreviewController::~RecordingPreviewController() {
    LOGI("RecordingPreviewController instance_ destroyed");
}

void RecordingPreviewController::PrepareEGLContext(ANativeWindow *window, JavaVM *g_jvm,
                                                   jobject obj, int screenWidth, int screenHeight,
                                                   int cameraFacingId) {
    LOGI("Creating RecordingPreviewController thread");
    this->vm_ = g_jvm;
    this->obj_ = obj;
    this->window_ = window;
    this->screen_width_ = screenWidth;
    this->screen_height_ = screenHeight;
    this->facing_id_ = cameraFacingId;
    handler_->PostMessage(new Message(MSG_EGL_THREAD_CREATE));
    int resCode = pthread_create(&thread_id_, 0, ThreadStartCallback, this);
    if (resCode == 0) {
        thread_create_succeed_ = true;
    }
}

void *RecordingPreviewController::ThreadStartCallback(void *myself) {
    RecordingPreviewController *previewController = (RecordingPreviewController *) myself;
    previewController->ProcessMessage();
    pthread_exit(0);
    return 0;
}

void RecordingPreviewController::ProcessMessage() {
    bool renderingEnabled = true;
    while (renderingEnabled) {
        Message *msg = NULL;
        if (queue_->DequeueMessage(&msg, true) > 0) {
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                renderingEnabled = false;
            }
            delete msg;
        }
    }
}

void RecordingPreviewController::NotifyFrameAvailable() {
    if (handler_ && !switch_camera_)
        handler_->PostMessage(new Message(MSG_RENDER_FRAME));
}

void RecordingPreviewController::StartEncoding(
        int width, int height, int videoBitRate,
        float frameRate, bool useHardWareEncoding,
        int strategy) {
    if (NULL != encoder_) {
        delete encoder_;
        encoder_ = NULL;
    }
    if (useHardWareEncoding) {
        encoder_ = new HWEncoderAdapter(vm_, obj_);
    } else {
        encoder_ = new SoftEncoderAdapter(strategy);
    }

    encoder_->init(width, height, videoBitRate, frameRate);
    if (handler_) {
        handler_->PostMessage(new Message(MSG_START_RECORDING));
    }
}

void RecordingPreviewController::StopEncoding() {
    LOGI("StopEncoding");
    if (handler_) {
        handler_->PostMessage(new Message(MSG_STOP_RECORDING));
    }
}

void RecordingPreviewController::AdaptiveVideoQuality(int maxBitRate, int avgBitRate, int fps) {
    if (encoder_) {
        encoder_->ReConfigure(maxBitRate, avgBitRate, fps);
    }
}

void RecordingPreviewController::SwitchCameraFacing() {
    LOGI("RecordingPreviewController::refereshCameraFacing");
    switch_camera_ = true;
    /*Notify render thread that camera has changed*/
    facing_id_ = facing_id_ == CAMERA_FACING_BACK ? CAMERA_FACING_FRONT : CAMERA_FACING_BACK;
    if (handler_) {
        handler_->PostMessage(new Message(MSG_SWITCH_CAMERA_FACING));
    }
    LOGI("leave RecordingPreviewController::refereshCameraFacing");
}

void RecordingPreviewController::ResetRenderSize(int screenWidth, int screenHeight) {
    LOGI("RecordingPreviewController::resetSize screen_width_:%d; screen_height_:%d", screenWidth,
         screenHeight);
    this->screen_width_ = screenWidth;
    this->screen_height_ = screenHeight;
}

void RecordingPreviewController::DestroyEGLContext() {
    LOGI("Stopping RecordingPreviewController thread");
    if (handler_) {
        handler_->PostMessage(new Message(MSG_EGL_THREAD_EXIT));
        handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    }

    if (thread_create_succeed_) {
        pthread_join(thread_id_, 0);
    }
    if (queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = NULL;
    }
    if (handler_) {
        delete handler_;
        handler_ = NULL;
    }
    LOGI("RecordingPreviewController thread stopped");
}

void RecordingPreviewController::CreateWindowSurface(ANativeWindow *window) {
    LOGI("enter RecordingPreviewController::CreateWindowSurface");
    if (this->window_ == NULL) {
        this->window_ = window;
        if (handler_) {
            handler_->PostMessage(new Message(MSG_EGL_CREATE_PREVIEW_SURFACE));
        }
    }
}

void RecordingPreviewController::DestroyWindowSurface() {
    LOGI("enter RecordingPreviewController::DestroyWindowSurface");
    if (handler_) {
        handler_->PostMessage(new Message(MSG_EGL_DESTROY_PREVIEW_SURFACE));
    }
}

void RecordingPreviewController::BuildRenderInstance() {
    renderer_ = new RecordingPreviewRenderer();
}

bool RecordingPreviewController::Initialize() {
    LOGI("Initializing context_");
    egl_core_ = new EGLCore();
    egl_core_->Init();
    this->CreatePreviewSurface();

    this->BuildRenderInstance();
    this->ConfigCamera();

    renderer_->Init(degress_, facing_id_ == CAMERA_FACING_FRONT, texture_width_, texture_height_,
                    camera_width_, camera_height_);
    this->StartCameraPreview();

    switch_camera_ = false;

    LOGI("leave Initializing context_");
    return true;
}

void RecordingPreviewController::CreatePreviewSurface() {
    preview_surface_ = egl_core_->CreateWindowSurface(window_);
    if (preview_surface_ != NULL) {
        egl_core_->MakeCurrent(preview_surface_);
    }
}

void RecordingPreviewController::DestroyPreviewSurface() {
    if (EGL_NO_SURFACE != preview_surface_) {
        egl_core_->ReleaseSurface(preview_surface_);
        preview_surface_ = EGL_NO_SURFACE;
        if (window_) {
            LOGI("VideoOutput Releasing surfaceWindow");
            ANativeWindow_release(window_);
            window_ = NULL;
        }
    }
}

void RecordingPreviewController::SwitchCamera() {
    this->ReleaseCamera();
    this->ConfigCamera();
    renderer_->SetDegress(degress_, facing_id_ == CAMERA_FACING_FRONT);
    this->StartCameraPreview();
    switch_camera_ = false;
}

void RecordingPreviewController::Destroy() {
    LOGI("enter RecordingPreviewController::Destroy...");
    this->DestroyPreviewSurface();
    if (renderer_) {
        renderer_->Destroy();
        delete renderer_;
        renderer_ = NULL;
    }
    this->ReleaseCamera();
    egl_core_->Release();
    delete egl_core_;
    egl_core_ = NULL;
    LOGI("leave RecordingPreviewController::Destroy...");
}

void RecordingPreviewController::StartRecording() {
    encoder_->CreateEncoder(egl_core_, renderer_->GetInputTexId());
    encoding_ = true;
}

void RecordingPreviewController::StopRecording() {
    LOGI("StopRecording....");
    encoding_ = false;
    if (encoder_) {
        encoder_->DestroyEncoder();
        delete encoder_;
        encoder_ = NULL;
    }
}

void RecordingPreviewController::RenderFrame() {
    if (NULL != egl_core_ && !switch_camera_) {
//		long start_time_mills_ = getCurrentTime();
        if (start_time_ == -1) {
            start_time_ = getCurrentTime();
        }
        float position = ((float) (getCurrentTime() - start_time_)) / 1000.0f;
        this->ProcessVideoFrame(position);
        if (preview_surface_ != EGL_NO_SURFACE) {
            this->Draw();
        }
//			LOGI("process Frame waste TimeMills 【%d】", (int)(getCurrentTime() - start_time_mills_));
        if (encoding_) {
            encoder_->Encode();
        }
    }
}

void RecordingPreviewController::Draw() {
    egl_core_->MakeCurrent(preview_surface_);
    renderer_->DrawToViewWithAutoFit(screen_width_, screen_height_, texture_width_, texture_height_);
    if (!egl_core_->SwapBuffers(preview_surface_)) {
        LOGE("eglSwapBuffers(preview_surface_) returned error %d", eglGetError());
    }
}

void RecordingPreviewController::ProcessVideoFrame(float position) {
    UpdateTexImage();
    renderer_->ProcessFrame(position);
}

void RecordingPreviewController::UpdateTexImage() {
//	LOGI("RecordingPreviewController::UpdateTexImage");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(obj_);
    if (NULL != jcls) {
        jmethodID updateTexImageCallback = env->GetMethodID(jcls, "updateTexImageFromNative",
                                                            "()V");
        if (NULL != updateTexImageCallback) {
            env->CallVoidMethod(obj_, updateTexImageCallback);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
//	LOGI("leave RecordingPreviewController::UpdateTexImage");
}

void RecordingPreviewController::ConfigCamera() {
    LOGI("RecordingPreviewController::ConfigCamera");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(obj_);
    if (NULL != jcls) {
        jmethodID configCameraCallback = env->GetMethodID(jcls, "configCameraFromNative",
                                                          "(I)Lcom/trinity/camera/CameraConfigInfo;");
        if (NULL != configCameraCallback) {
            jobject cameraConfigInfo = env->CallObjectMethod(obj_, configCameraCallback, facing_id_);
            jclass cls_CameraConfigInfo = env->GetObjectClass(cameraConfigInfo);
            jmethodID cameraConfigInfo_getDegress = env->GetMethodID(cls_CameraConfigInfo,
                                                                     "getDegress", "()I");
            degress_ = env->CallIntMethod(cameraConfigInfo, cameraConfigInfo_getDegress);

            jmethodID cameraConfigInfo_getCameraFacingId = env->GetMethodID(cls_CameraConfigInfo,
                                                                            "getCameraFacingId",
                                                                            "()I");
            facing_id_ = env->CallIntMethod(cameraConfigInfo, cameraConfigInfo_getCameraFacingId);

            jmethodID cameraConfigInfo_getTextureWidth = env->GetMethodID(cls_CameraConfigInfo,
                                                                          "getTextureWidth", "()I");
            int previewWidth = env->CallIntMethod(cameraConfigInfo,
                                                  cameraConfigInfo_getTextureWidth);
            jmethodID cameraConfigInfo_getTextureHeight = env->GetMethodID(cls_CameraConfigInfo,
                                                                           "getTextureHeight",
                                                                           "()I");
            int previewHeight = env->CallIntMethod(cameraConfigInfo,
                                                   cameraConfigInfo_getTextureHeight);

            this->camera_width_ = previewWidth;
            this->camera_height_ = previewHeight;

            texture_width_ = 360;
            texture_height_ = 640;
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void RecordingPreviewController::ReleaseCamera() {
    LOGI("RecordingPreviewController::ReleaseCamera");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(obj_);
    if (NULL != jcls) {
        jmethodID releaseCameraCallback = env->GetMethodID(jcls, "releaseCameraFromNative", "()V");
        if (NULL != releaseCameraCallback) {
            env->CallVoidMethod(obj_, releaseCameraCallback);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void RecordingPreviewController::StartCameraPreview() {
    LOGI("RecordingPreviewController::setCameraPreviewTexture");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(obj_);
    if (NULL != jcls) {
        jmethodID startPreviewCallback = env->GetMethodID(jcls, "startPreviewFromNative", "(I)V");
        if (NULL != startPreviewCallback) {
            env->CallVoidMethod(obj_, startPreviewCallback, renderer_->GetCameraTexId());
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void RecordingPreviewController::HotConfigQuality(int maxBitrate, int avgBitrate, int fps) {
    if (encoder_) {
        encoder_->HotConfig(maxBitrate, avgBitrate, fps);
    }
}

void RecordingPreviewController::HotConfig(int bitRate, int fps, int gopSize) {
    LOGI("RecordingPreviewController::HotConfig");
    if (encoder_) {
        encoder_->HotConfig(bitRate, fps, gopSize);
    }
}


