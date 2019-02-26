//
// Created by wlanjie on 2019/2/21.
//

#include "preview_controller.h"
#include "../log.h"
#include "../common/common_tools.h"

namespace trinity {

PreviewController::PreviewController() {
    facing_id_ = CAMERA_FACING_FRONT;
    start_time_ = 0;
    egl_core_ = nullptr;
    window_ = nullptr;
    is_thread_create_succeed_ = false;

    preview_surface_ = EGL_NO_SURFACE;
    queue_ = new MessageQueue("PreviewController message queue");
    handler = new RecordingPreviewHandler(this, queue_);
}

PreviewController::~PreviewController() {

}

void PreviewController::PrepareEGLContext(ANativeWindow *window, JavaVM *vm, jobject obj, int screen_width,
                                          int screen_height, int camera_facing_id) {
    vm_ = vm;
    object_ = obj;
    window_ = window;
    screen_width_ = screen_width;
    screen_height_ = screen_height;
    facing_id_ = camera_facing_id;
    handler->PostMessage(new Message(MSG_EGL_THREAD_CREATE));
    int result = pthread_create(&thread_id_, 0, ThreadStartCallback, this);
    if (result == 0) {
        is_thread_create_succeed_ = true;
    }
}

void PreviewController::NotifyFrameAvailable() {
    if (handler && !is_switching_camera_) {
        handler->PostMessage(new Message(MSG_RENDER_FRAME));
    }
}

void PreviewController::SwitchCameraFacing() {
    is_switching_camera_ = true;
    if (facing_id_ == CAMERA_FACING_BACK) {
        facing_id_ = CAMERA_FACING_FRONT;
    } else {
        facing_id_ = CAMERA_FACING_BACK;
    }
    if (handler) {
        handler->PostMessage(new Message(MSG_SWITCH_CAMERA_FACING));
    }
}

void PreviewController::AdaptiveVideoQuality(int max_bit_rate, int avg_bit_rate, int fps) {

}

void PreviewController::ResetRenderSize(int screen_width, int screen_height) {
    screen_width_ = screen_width;
    screen_height_ = screen_height;
}

void PreviewController::DestroyEGLContext() {
    if (handler) {
        handler->PostMessage(new Message(MSG_EGL_THREAD_EXIT));
        handler->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    }
    if (is_thread_create_succeed_) {
        pthread_join(thread_id_, 0);
    }
    if (queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = nullptr;
    }
    if (handler) {
        delete handler;
        handler = nullptr;
    }
}

void *PreviewController::ThreadStartCallback(void *my_self) {
    PreviewController* controller = (PreviewController*)(my_self);
    controller->ProcessMessage();
    pthread_exit(0);
    return nullptr;
}

void PreviewController::ProcessMessage() {
    bool rendering_enabled = true;
    while (rendering_enabled) {
        Message* msg = nullptr;
        if (queue_->DequeueMessage(&msg, true) > 0) {
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->execute()) {
                rendering_enabled = false;
            }
            delete msg;
        }
    }
}

void PreviewController::BuildRenderInstance() {
    renderer_ = new PreviewRenderer();
}

void PreviewController::ProcessVideoFrame(float position) {
    UpdateTextureImage();
    renderer_->ProcessFrame(position);
}

void PreviewController::Draw() {
    egl_core_->MakeCurrent(preview_surface_);
    renderer_->DrawToViewWithAutoFit(screen_width_, screen_height_, texture_width_, texture_height_);
    if (!egl_core_->SwapBuffers(preview_surface_)) {
        LOGE("eglSwapBuffers(previewSurface) returned error %d", eglGetError());
    }
}

void PreviewController::ConfigCamera() {
    LOGI("MVRecordingPreviewController::configCamera");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(object_);
    if (NULL != jcls) {
        jmethodID configCameraCallback = env->GetMethodID(jcls, "configCameraFromNative",
                                                          "(I)Lcom/trinity/camera/CameraConfigInfo;");
        if (NULL != configCameraCallback) {
            jobject cameraConfigInfo = env->CallObjectMethod(object_, configCameraCallback, facing_id_);
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

//			int previewMin = MIN(previewWidth, previewHeight);
//			textureWidth = previewMin >= 480 ? 480 : previewMin;
//			textureHeight = textureWidth;

            texture_width_ = 360;
            texture_height_ = 640;
//			textureWidth = 720;
//			textureHeight = 1280;
            LOGI("camera : {%d, %d}", previewWidth, previewHeight);
//            LOGI("Texture : {%d, %d}", textureWidth, textureHeight);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void PreviewController::StartCameraPreview() {
    LOGI("MVRecordingPreviewController::setCameraPreviewTexture");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(object_);
    if (NULL != jcls) {
        jmethodID startPreviewCallback = env->GetMethodID(jcls, "startPreviewFromNative", "(I)V");
        if (NULL != startPreviewCallback) {
            env->CallVoidMethod(object_, startPreviewCallback, renderer_->GetCameraTextureId());
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void PreviewController::UpdateTextureImage() {
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed: %s", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(object_);
    if (NULL != jcls) {
        jmethodID updateTexImageCallback = env->GetMethodID(jcls, "updateTexImageFromNative",
                                                            "()V");
        if (NULL != updateTexImageCallback) {
            env->CallVoidMethod(object_, updateTexImageCallback);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
}

void PreviewController::ReleaseCamera() {
    LOGI("MVRecordingPreviewController::releaseCamera");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass jcls = env->GetObjectClass(object_);
    if (NULL != jcls) {
        jmethodID releaseCameraCallback = env->GetMethodID(jcls, "releaseCameraFromNative", "()V");
        if (NULL != releaseCameraCallback) {
            env->CallVoidMethod(object_, releaseCameraCallback);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void PreviewController::StartEncoding(int width, int height, int video_bit_rate, float frame_rate,
                                      bool use_hard_ware_encoding, int strategy) {

}

void PreviewController::StopEncoding() {

}

void PreviewController::CreateWindowSurface(ANativeWindow *window) {
    if (window_ == nullptr) {
        window_ = window;
        if (handler) {
            handler->PostMessage(new Message(MSG_EGL_CREATE_PREVIEW_SURFACE));
        }
    }
}

void PreviewController::DestroyWindowSurface() {
    if (handler) {
        handler->PostMessage(new Message(MSG_EGL_DESTROY_PREVIEW_SURFACE));
    }
}

bool PreviewController::Initialize() {
    const EGLint attribs[] = {EGL_BUFFER_SIZE, 32, EGL_ALPHA_SIZE, 8, EGL_BLUE_SIZE, 8,
                              EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,
                              EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE};
    LOGI("Initializing context");
    egl_core_ = new EGLCore();
    egl_core_->Init();
    this->CreatePreviewSurface();

    this->BuildRenderInstance();
    this->ConfigCamera();

    renderer_->Init(degress_, facing_id_ == CAMERA_FACING_FRONT, texture_width_, texture_height_, camera_width_, camera_height_);
    this->StartCameraPreview();

    is_switching_camera_ = false;

    LOGI("leave Initializing context");
    return true;
}

void PreviewController::Destroy() {
    LOGI("enter MVRecordingPreviewController::destroy...");
    this->DestroyPreviewSurface();
    if (renderer_) {
        renderer_->Dealloc();
        delete renderer_;
        renderer_ = nullptr;
    }
    this->ReleaseCamera();
    egl_core_->Release();
    delete egl_core_;
    egl_core_ = nullptr;
    LOGI("leave MVRecordingPreviewController::destroy...");
}

void PreviewController::CreatePreviewSurface() {
    preview_surface_ = egl_core_->CreateWindowSurface(window_);
    if (preview_surface_ != nullptr) {
        egl_core_->MakeCurrent(preview_surface_);
    }
}

void PreviewController::DestroyPreviewSurface() {
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

void PreviewController::SwitchCamera() {
    this->ReleaseCamera();
    this->ConfigCamera();
    renderer_->SetDegress(degress_, facing_id_ == CAMERA_FACING_FRONT);
    this->StartCameraPreview();
    is_switching_camera_ = false;
}

void PreviewController::StartRecording() {

}

void PreviewController::StopRecording() {

}

void PreviewController::RenderFrame() {
    if (nullptr != egl_core_ && !is_switching_camera_) {
//		long startTimeMills = getCurrentTime();
        if (start_time_ == -1) {
            start_time_ = getCurrentTime();
        }
        float position = ((float) (getCurrentTime() - start_time_)) / 1000.0f;
        this->ProcessVideoFrame(position);
        if (preview_surface_ != EGL_NO_SURFACE) {
            this->Draw();
        }
//			LOGI("process Frame waste TimeMills 【%d】", (int)(getCurrentTime() - startTimeMills));
//        if (isEncoding) {
//            encoder->encode();
//        }
    }
}

}