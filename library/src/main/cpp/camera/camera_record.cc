//
// Created by wlanjie on 2019/4/13.
//

#include "gl.h"
#include "camera_record.h"
#include "media_encode_adapter.h"
#include "tools.h"
#include "android_xlog.h"

namespace trinity {

CameraRecord::CameraRecord() {
    window_ = nullptr;
    vm_ = nullptr;
    screen_height_ = 0;
    screen_width_ = 0;
    switch_camera_ = false;
    facing_id_ = CAMERA_FACING_FRONT;
    camera_width_ = 0;
    camera_height_ = 0;
    handler_ = nullptr;
    queue_ = nullptr;
    egl_core_ = nullptr;
    render_screen_ = nullptr;
    preview_surface_ = EGL_NO_SURFACE;
    texture_matrix_ = nullptr;
    frame_buffer_ = nullptr;
    render_screen_ = nullptr;
    oes_texture_id_ = 0;
    encoder_ = nullptr;
    encoding_ = false;
    packet_thread_ = nullptr;
}

CameraRecord::~CameraRecord() {

}

void
CameraRecord::PrepareEGLContext(ANativeWindow *window,
        JavaVM *vm, jobject object,
        int screen_width, int screen_height, int camera_id) {
    this->vm_ = vm;
    this->obj_ = object;
    this->window_ = window;
    this->screen_width_ = screen_width;
    this->screen_height_ = screen_height;
    this->facing_id_ = camera_id;
    packet_thread_ = new VideoConsumerThread();
    queue_ = new MessageQueue("CameraRecord message queue");
    handler_ = new CameraRecordHandler(this, queue_);
    handler_->PostMessage(new Message(MSG_EGL_THREAD_CREATE));
    pthread_create(&thread_id_, 0, ThreadStartCallback, this);
}

void CameraRecord::NotifyFrameAvailable() {
    if (nullptr != handler_ && !switch_camera_) {
        handler_->PostMessage(new Message(MSG_RENDER_FRAME));
    }
}

void CameraRecord::SetFrame(int frame) {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_SET_FRAME, frame, 0));
    }
}

void CameraRecord::SwitchCameraFacing() {
    switch_camera_ = true;
    facing_id_ = facing_id_ == CAMERA_FACING_BACK ? CAMERA_FACING_FRONT : CAMERA_FACING_BACK;
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_SWITCH_CAMERA_FACING));
    }
}

void CameraRecord::ResetRenderSize(int screen_width, int screen_height) {
    screen_width_ = screen_width;
    screen_height_ = screen_height;
}

void CameraRecord::DestroyEGLContext() {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_EGL_THREAD_EXIT));
        handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    }
    pthread_join(thread_id_, 0);
    if (nullptr != queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = nullptr;
    }
    if (nullptr != handler_) {
        delete handler_;
        handler_ = nullptr;
    }
    if (nullptr != packet_thread_) {
        delete packet_thread_;
        packet_thread_ = nullptr;
    }
}

void CameraRecord::ProcessVideoFrame(float position) {
    UpdateTexImage();
}

void CameraRecord::Draw() {
    egl_core_->MakeCurrent(preview_surface_);
    UpdateTexImage();
    GetTextureMatrix();
    render_screen_->SetOutput(screen_width_, screen_height_);
    frame_buffer_->SetOutput(MIN(camera_width_, camera_height_), MAX(camera_width_, camera_height_));
    int texture_id = OnDrawFrame(oes_texture_id_, camera_width_, camera_height_);
    frame_buffer_->SetTextureType(texture_id > 0 ? TEXTURE_2D : TEXTURE_OES);
    texture_id = frame_buffer_->OnDrawFrame(texture_id > 0 ? texture_id : oes_texture_id_, texture_matrix_);
    render_screen_->ProcessImage(texture_id);
    if (!egl_core_->SwapBuffers(preview_surface_)) {
        LOGE("eglSwapBuffers(preview_surface_) returned error %d", eglGetError());
    }
}

void CameraRecord::ConfigCamera() {
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == NULL) {
        LOGE("getJNIEnv failed");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (NULL != clazz) {
        jmethodID preview_width = env->GetMethodID(clazz, "getCameraWidth", "()I");
        if (nullptr != preview_width) {
            this->camera_width_ = env->CallIntMethod(obj_, preview_width);
        }

        jmethodID preview_height = env->GetMethodID(clazz, "getCameraHeight", "()I");
        if (nullptr != preview_height) {
            this->camera_height_ = env->CallIntMethod(obj_, preview_height);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void CameraRecord::StartCameraPreview() {
    JNIEnv* env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("AttachCurrentThread failed.");
        return;
    }
    if (nullptr == env) {
        LOGE("JNIEnv init failed.");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID start_preview_callback = env->GetMethodID(clazz, "startPreviewFromNative", "(I)V");
        if (nullptr != start_preview_callback) {
            env->CallVoidMethod(obj_, start_preview_callback, oes_texture_id_);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("DetachCurrentThread failed.");
    }
}

void CameraRecord::UpdateTexImage() {
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (nullptr == env) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID updateTexImageCallback = env->GetMethodID(clazz, "updateTexImageFromNative", "()V");
        if (nullptr != updateTexImageCallback) {
            env->CallVoidMethod(obj_, updateTexImageCallback);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
}

void CameraRecord::onSurfaceCreated() {
    JNIEnv* env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("AttachCurrentThread failed");
        return;
    }
    if (nullptr == env) {
        LOGE("getJNIEnv failed");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID onSurfaceCreated = env->GetMethodID(clazz, "onSurfaceCreated", "()V");
        if (nullptr != onSurfaceCreated) {
            env->CallVoidMethod(obj_, onSurfaceCreated);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("DetachCurrentThread failed");
    }
}

int CameraRecord::OnDrawFrame(int texture_id, int width, int height) {
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return 0;
    }
    if (nullptr == env) {
        LOGI("getJNIEnv failed");
        return 0;
    }
    int id = 0;
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID onDrawFrame = env->GetMethodID(clazz, "onDrawFrame", "(III)I");
        if (nullptr != onDrawFrame) {
            id = env->CallIntMethod(obj_, onDrawFrame, texture_id, width, height);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
    return id;
}

void CameraRecord::onSurfaceDestroy() {
    JNIEnv* env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("AttachCurrentThread failed");
        return;
    }
    if (nullptr == env) {
        LOGE("getJNIEnv failed");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID onSurfaceDestroy = env->GetMethodID(clazz, "onSurfaceDestroy", "()V");
        if (nullptr != onSurfaceDestroy) {
            env->CallVoidMethod(obj_, onSurfaceDestroy);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("DetachCurrentThread failed");
    }
}

void CameraRecord::GetTextureMatrix() {
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (nullptr == env) {
        LOGI("getJNIEnv failed");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID getTextureMatrix = env->GetMethodID(clazz, "getTextureMatrix", "()[F");
        if (nullptr != getTextureMatrix) {
            jfloatArray matrix = static_cast<jfloatArray>(env->CallObjectMethod(obj_, getTextureMatrix));
            float* texture_matrix = env->GetFloatArrayElements(matrix, JNI_FALSE);
            memcpy(texture_matrix_, texture_matrix, sizeof(float) * 16);
            env->ReleaseFloatArrayElements(matrix, texture_matrix, 0);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
}

void CameraRecord::ReleaseCamera() {
    JNIEnv* env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("AttachCurrentThread failed.");
        return;
    }
    if (nullptr == env)  {
        LOGE("JNIEnv init failed.");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID releaseCameraCallback = env->GetMethodID(clazz, "releaseCameraFromNative", "()V");
        if (NULL != releaseCameraCallback) {
            env->CallVoidMethod(obj_, releaseCameraCallback);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        return;
    }
}

void *CameraRecord::ThreadStartCallback(void *myself) {
    CameraRecord *record = (CameraRecord *) myself;
    record->ProcessMessage();
    pthread_exit(0);
}

void CameraRecord::ProcessMessage() {
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

bool CameraRecord::Initialize() {
    texture_matrix_ = new GLfloat[16];
    egl_core_ = new EGLCore();
    egl_core_->Init();
    CreatePreviewSurface();

    glGenTextures(1, &oes_texture_id_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, oes_texture_id_);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    StartCameraPreview();
    ConfigCamera();
    frame_buffer_ = new FrameBuffer(MIN(camera_width_, camera_height_), MAX(camera_width_, camera_height_), DEFAULT_VERTEX_MATRIX_SHADER, DEFAULT_OES_FRAGMENT_SHADER);
    render_screen_ = new OpenGL(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    switch_camera_ = false;
    return true;
}

void CameraRecord::Destroy() {
    if (oes_texture_id_ != 0) {
        glDeleteTextures(1, &oes_texture_id_);
        oes_texture_id_ = 0;
    }
    if (nullptr != frame_buffer_) {
        delete frame_buffer_;
        frame_buffer_ = nullptr;
    }
    if (nullptr != render_screen_) {
        delete render_screen_;
        render_screen_ = nullptr;
    }

    DestroyPreviewSurface();
    if (nullptr != render_screen_) {
        // TODO
        delete render_screen_;
        render_screen_ = nullptr;
    }
    ReleaseCamera();
    if (nullptr != egl_core_) {
        egl_core_->Release();
        delete egl_core_;
        egl_core_ = nullptr;
    }
    if (nullptr != texture_matrix_) {
        delete[] texture_matrix_;
        texture_matrix_ = nullptr;
    }
}

void CameraRecord::CreateWindowSurface(ANativeWindow *window) {
    if (nullptr != window_) {
        window_ = window;
        if (nullptr != handler_) {
            handler_->PostMessage(new Message(MSG_EGL_CREATE_PREVIEW_SURFACE));
        }
    }
}

void CameraRecord::DestroyWindowSurface() {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_EGL_DESTROY_PREVIEW_SURFACE));
    }
}

void CameraRecord::CreatePreviewSurface() {
    if (nullptr != window_) {
        preview_surface_ = egl_core_->CreateWindowSurface(window_);
        if (nullptr != preview_surface_) {
            egl_core_->MakeCurrent(preview_surface_);
        }

        onSurfaceCreated();
    }
}

void CameraRecord::DestroyPreviewSurface() {
    if (EGL_NO_SURFACE != preview_surface_) {
        onSurfaceDestroy();

        egl_core_->ReleaseSurface(preview_surface_);
        preview_surface_ = EGL_NO_SURFACE;
        if (nullptr != window_) {
            ANativeWindow_release(window_);
            window_ = nullptr;
        }
    }
}

void CameraRecord::SwitchCamera() {
    ReleaseCamera();
    ConfigCamera();

}

void CameraRecord::StartEncoding(const char* path,
                   int width,
                   int height,
                   int video_bit_rate,
                   int frame_rate,
                   bool use_hard_encode,
                   int audio_sample_rate,
                   int audio_channel,
                   int audio_bit_rate) {
    LOGI("StartEncoding enter width: %d height: %d videoBitRate: %d frameRate: %f useHard: %d audio_sample_rate: %d audio_channel: %d audio_bit_rate: %d",
            width, height, video_bit_rate, frame_rate, use_hard_encode, audio_sample_rate, audio_channel, audio_bit_rate);

    if (nullptr != encoder_) {
        delete encoder_;
        encoder_ = nullptr;
    }
    if (nullptr != packet_thread_) {
        PacketPool::GetInstance()->InitRecordingVideoPacketQueue();
        PacketPool::GetInstance()->InitAudioPacketQueue(44100);
        AudioPacketPool::GetInstance()->InitAudioPacketQueue();
        int ret = packet_thread_->Init(path, width, height, frame_rate, video_bit_rate, audio_sample_rate, audio_channel, audio_bit_rate, "libfdk_aac");
        if (ret >= 0) {
            packet_thread_->StartAsync();
        }
    }
    if (use_hard_encode) {
        encoder_ = new MediaEncodeAdapter(vm_, obj_);
    } else {
        //
        encoder_ = new SoftEncoderAdapter(0);
    }
    encoder_->Init(width, height, video_bit_rate, frame_rate);
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_START_RECORDING));
    }
}

void CameraRecord::StartRecording() {
    if (nullptr != encoder_) {
        encoder_->CreateEncoder(egl_core_, frame_buffer_->GetTextureId());
        encoding_ = true;
    }
}

void CameraRecord::StopEncoding() {
    LOGI("StopEncoding");
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_STOP_RECORDING));
    }
}

void CameraRecord::StopRecording() {
    LOGI("StopRecording");
    encoding_ = false;
    if (nullptr != encoder_) {
        encoder_->DestroyEncoder();
        delete encoder_;
        encoder_ = nullptr;
    }
    if (nullptr != packet_thread_) {
        packet_thread_->Stop();
        PacketPool::GetInstance()->DestroyRecordingVideoPacketQueue();
        PacketPool::GetInstance()->DestroyAudioPacketQueue();
        AudioPacketPool::GetInstance()->DestroyAudioPacketQueue();
    }
}

void CameraRecord::RenderFrame() {
    if (nullptr != egl_core_ && !switch_camera_) {
//        if (start_time_ == 0) {
//            start_time_ = getCurrentTime();
//        }
//        float position = ((float) (getCurrentTime() - start_time_)) / 1000.0f;
//        ProcessVideoFrame(position);
        if (EGL_NO_SURFACE != preview_surface_) {
            Draw();
        }

        if (encoding_ && nullptr != encoder_) {
            encoder_->Encode();
        }
    }
}

void CameraRecord::SetFrameType(int frame) {
    frame_buffer_->SetFrame(frame, screen_width_, screen_height_);
}

}