//
// Created by wlanjie on 2019/4/13.
//

#include "gl.h"
#include "camera_record.h"
#include "media_encode_adapter.h"
#include "tools.h"
#include "android_xlog.h"

namespace trinity {

CameraRecord::CameraRecord(JNIEnv* env) {
    window_ = nullptr;
    env_ = env;
    vm_ = nullptr;
    env->GetJavaVM(&vm_);
    screen_height_ = 0;
    screen_width_ = 0;
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
    start_time_ = 0;
    speed_ = 1.0f;
    render_type_ = CROP;

    // 因为encoder_render时不能改变顶点和纹理坐标
    // 而glReadPixels读取的图像又是上下颠倒的
    // 所以这里显示的把纹理坐标做180度旋转
    // 从而保证从glReadPixels读取的数据不是上下颠倒的,而是正确的
    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];
    vertex_coordinate_[0] = -1.0f;
    vertex_coordinate_[1] = -1.0f;
    vertex_coordinate_[2] = 1.0f;
    vertex_coordinate_[3] = -1.0f;

    vertex_coordinate_[4] = -1.0f;
    vertex_coordinate_[5] = 1.0f;
    vertex_coordinate_[6] = 1.0f;
    vertex_coordinate_[7] = 1.0f;

    texture_coordinate_[0] = 0.0f;
    texture_coordinate_[1] = 1.0f;
    texture_coordinate_[2] = 1.0f;
    texture_coordinate_[3] = 1.0f;

    texture_coordinate_[4] = 0.0f;
    texture_coordinate_[5] = 0.0f;
    texture_coordinate_[6] = 1.0f;
    texture_coordinate_[7] = 0.0f;
}

CameraRecord::~CameraRecord() {
    LOGI("enter ~CameraRecord");
    delete[] vertex_coordinate_;
    vertex_coordinate_ = nullptr;
    delete[] texture_coordinate_;
    texture_coordinate_ = nullptr;
    LOGI("leave ~CameraRecord");
}

void CameraRecord::PrepareEGLContext(
        jobject object, jobject surface,
        int screen_width, int screen_height) {
    LOGI("enter PrepareEGLContext");
    this->obj_ = env_->NewGlobalRef(object);
    this->window_ = ANativeWindow_fromSurface(env_, surface);
    this->screen_width_ = screen_width;
    this->screen_height_ = screen_height;
    packet_thread_ = new VideoConsumerThread();
    queue_ = new MessageQueue("CameraRecord message queue");
    handler_ = new CameraRecordHandler(this, queue_);
    handler_->PostMessage(new Message(MSG_EGL_THREAD_CREATE));
    pthread_create(&thread_id_, 0, ThreadStartCallback, this);
    LOGI("leave PrepareEGLContext");
}

void CameraRecord::NotifyFrameAvailable() {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_RENDER_FRAME));
    }
}

void CameraRecord::SetSpeed(float speed) {
    speed_ = speed;
}

void CameraRecord::SetRenderType(int type) {
    LOGI("SetRenderType: %d", type);
    if (nullptr != frame_buffer_) {
        frame_buffer_->SetRenderType(type);
    }
    render_type_ = type;
}

void CameraRecord::SetFrame(int frame) {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_SET_FRAME, frame, 0));
    }
}

void CameraRecord::SwitchCameraFacing() {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_SWITCH_CAMERA_FACING));
    }
}

void CameraRecord::ResetRenderSize(int screen_width, int screen_height) {
    screen_width_ = screen_width;
    screen_height_ = screen_height;
}

void CameraRecord::DestroyEGLContext() {
    LOGI("%s enter", __FUNCTION__);
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
    if (nullptr != obj_) {
        env_->DeleteGlobalRef(obj_);
        obj_ = nullptr;
    }
    LOGI("%s leave", __FUNCTION__);
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
    LOGI("enter ConfigCamera");
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
    LOGI("leave ConfigCamera");
}

void CameraRecord::StartCameraPreview() {
    LOGI("enter StartCameraPreview");
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
    LOGI("leave StartCameraPreview");
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
    LOGI("enter %s", __FUNCTION__);
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
    LOGI("leave %s", __FUNCTION__);
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
    LOGI("enter onSurfaceDestroy");
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
    LOGI("leave onSurfaceDestroy");
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
    LOGI("enter %s", __FUNCTION__);
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
    LOGI("leave %s", __FUNCTION__);
}

void *CameraRecord::ThreadStartCallback(void *myself) {
    CameraRecord *record = (CameraRecord *) myself;
    record->ProcessMessage();
    pthread_exit(0);
}

void CameraRecord::ProcessMessage() {
    LOGI("enter %s", __FUNCTION__);
    bool renderingEnabled = true;
    while (renderingEnabled) {
        Message *msg = nullptr;
        if (queue_->DequeueMessage(&msg, true) > 0) {
            if (nullptr != msg) {
                if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                    renderingEnabled = false;
                }
                delete msg;
            }
        }
    }
    LOGI("leave %s", __FUNCTION__);
}

bool CameraRecord::Initialize() {
    LOGI("enter %s", __FUNCTION__);
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
    frame_buffer_ = new FrameBuffer(render_type_, MIN(camera_width_, camera_height_), MAX(camera_width_, camera_height_),
            screen_width_, screen_height_, DEFAULT_VERTEX_MATRIX_SHADER, DEFAULT_OES_FRAGMENT_SHADER);
    render_screen_ = new OpenGL(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    LOGI("leave %s", __FUNCTION__);
    return true;
}

void CameraRecord::Destroy() {
    LOGI("enter %s", __FUNCTION__);
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
    LOGI("leave %s", __FUNCTION__);
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
    LOGI("enter %s", __FUNCTION__);
    if (nullptr != window_) {
        preview_surface_ = egl_core_->CreateWindowSurface(window_);
        if (nullptr != preview_surface_) {
            egl_core_->MakeCurrent(preview_surface_);
        }

        onSurfaceCreated();
    }
    LOGI("leave %s", __FUNCTION__);
}

void CameraRecord::DestroyPreviewSurface() {
    LOGI("enter %s", __FUNCTION__);
    if (EGL_NO_SURFACE != preview_surface_) {
        onSurfaceDestroy();

        egl_core_->ReleaseSurface(preview_surface_);
        preview_surface_ = EGL_NO_SURFACE;
        if (nullptr != window_) {
            ANativeWindow_release(window_);
            window_ = nullptr;
        }
    }
    LOGI("leave %s", __FUNCTION__);
}

void CameraRecord::SwitchCamera() {
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
        encoder_ = new SoftEncoderAdapter(vertex_coordinate_, texture_coordinate_);
    }
    encoder_->Init(width, height, video_bit_rate, frame_rate);
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(MSG_START_RECORDING));
    }
}

void CameraRecord::StartRecording() {
    start_time_ = 0;
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
    if (nullptr != egl_core_) {
        if (start_time_ == 0) {
            start_time_ = getCurrentTime();
        }
//        float position = ((float) (getCurrentTime() - start_time_)) / 1000.0f;
//        ProcessVideoFrame(position);
        if (EGL_NO_SURFACE != preview_surface_) {
            Draw();
        }

        int64_t duration = getCurrentTime() - start_time_;
        if (encoding_ && nullptr != encoder_) {
            encoder_->Encode((int) (duration * speed_));
        }
    }
}

void CameraRecord::SetFrameType(int frame) {
    frame_buffer_->SetFrame(frame, screen_width_, screen_height_);
}

}