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

#include <string>
#include "gl.h"
#include "camera_record.h"
#include "media_encode_adapter.h"
#include "tools.h"
#include "android_xlog.h"
#include "trinity.h"

namespace trinity {

CameraRecord::CameraRecord(JNIEnv* env) : Handler()
    , window_(nullptr)
    , vm_(nullptr)
    , obj_(nullptr)
    , screen_width_(0)
    , screen_height_(0)
    , camera_width_(0)
    , camera_height_(0)
    , camera_size_change_(false)
    , vertex_coordinate_(nullptr)
    , texture_coordinate_(nullptr)
    , egl_core_(nullptr)
    , preview_surface_(EGL_NO_SURFACE)
    , frame_buffer_(nullptr)
    , render_screen_(nullptr)
    , queue_(nullptr)
    , thread_id_()
    , oes_texture_id_(0)
    , texture_matrix_(nullptr)
    , encoder_(nullptr)
    , encoding_(false)
    , packet_thread_(nullptr)
    , start_time_(0)
    , speed_(1.0F)
    , frame_type_(-1)
    , frame_count_(0)
    , pre_fps_count_time_(0)
    , fps_(1.0F)
    , start_recording(false)
    , current_action_id_(0)
    , image_process_(nullptr)
    , render_time_(0) {
    env->GetJavaVM(&vm_);
}

CameraRecord::~CameraRecord() = default;

void CameraRecord::PrepareEGLContext(
        jobject object, jobject surface,
        int screen_width, int screen_height) {
    LOGI("enter PrepareEGLContext");
    // 因为encoder_render时不能改变顶点和纹理坐标
    // 而glReadPixels读取的图像又是上下颠倒的
    // 所以这里显示的把纹理坐标做180度旋转
    // 从而保证从glReadPixels读取的数据不是上下颠倒的,而是正确的
    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];
    vertex_coordinate_[0] = -1.0F;
    vertex_coordinate_[1] = -1.0F;
    vertex_coordinate_[2] = 1.0F;
    vertex_coordinate_[3] = -1.0F;

    vertex_coordinate_[4] = -1.0F;
    vertex_coordinate_[5] = 1.0F;
    vertex_coordinate_[6] = 1.0F;
    vertex_coordinate_[7] = 1.0F;

    texture_coordinate_[0] = 0.0F;
    texture_coordinate_[1] = 1.0F;
    texture_coordinate_[2] = 1.0F;
    texture_coordinate_[3] = 1.0F;

    texture_coordinate_[4] = 0.0F;
    texture_coordinate_[5] = 0.0F;
    texture_coordinate_[6] = 1.0F;
    texture_coordinate_[7] = 0.0F;
    JNIEnv* env = nullptr;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        return;
    }
    this->obj_ = env->NewGlobalRef(object);
    this->window_ = ANativeWindow_fromSurface(env, surface);
    this->screen_width_ = screen_width;
    this->screen_height_ = screen_height;
    packet_thread_ = new VideoConsumerThread();
    queue_ = new MessageQueue("CameraRecord message queue");
    InitMessageQueue(queue_);
    PostMessage(new Message(MSG_EGL_THREAD_CREATE));
    pthread_create(&thread_id_, nullptr, ThreadStartCallback, this);
    LOGI("leave PrepareEGLContext");
}

void CameraRecord::NotifyFrameAvailable() {
    PostMessage(new Message(MSG_RENDER_FRAME));
}

void CameraRecord::SetSpeed(float speed) {
    LOGI("SetSpeed: %f", speed);
    speed_ = speed;
}

void CameraRecord::SetRenderType(int type) {
    LOGI("SetRenderType: %d", type);
    if (nullptr != frame_buffer_) {
//        frame_buffer_->SetRenderType(type);
    }
}

void CameraRecord::SetFrame(int frame) {
    LOGI("SetFrame: %d", frame);
    PostMessage(new Message(MSG_SET_FRAME, frame, 0));
}

void CameraRecord::SwitchCameraFacing() {
    PostMessage(new Message(MSG_SWITCH_CAMERA_FACING));
}

void CameraRecord::ResetRenderSize(int screen_width, int screen_height) {
    LOGI("%s screen_width: %d screen_height: %d", __FUNCTION__, screen_width, screen_height);
    screen_width_ = screen_width;
    screen_height_ = screen_height;
}

void CameraRecord::DestroyEGLContext() {
    LOGI("%s enter", __FUNCTION__);
    PostMessage(new Message(MSG_EGL_THREAD_EXIT));
    PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(thread_id_, 0);
    if (nullptr != queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = nullptr;
    }
    if (nullptr != packet_thread_) {
        delete packet_thread_;
        packet_thread_ = nullptr;
    }
    if (nullptr != obj_) {
        JNIEnv* env = nullptr;
        if (vm_->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            env->DeleteGlobalRef(obj_);
            obj_ = nullptr;
        }
    }
    if (nullptr != vertex_coordinate_) {
        delete[] vertex_coordinate_;
        vertex_coordinate_ = nullptr;
    }
    if (nullptr != texture_coordinate_) {
        delete[] texture_coordinate_;
        texture_coordinate_ = nullptr;
    }
    render_time_ = 0;
    LOGI("%s leave", __FUNCTION__);
}

void CameraRecord::Draw() {
    egl_core_->MakeCurrent(preview_surface_);
    UpdateTexImage();
    GetTextureMatrix();
    if (camera_width_ == 0 || camera_height_ == 0) {
        ConfigCamera();
    }
    if (camera_size_change_) {
        camera_size_change_ = false;
        if (nullptr != frame_buffer_) {
            delete frame_buffer_;
            frame_buffer_ = nullptr;
        }
    }
    if (frame_buffer_ == nullptr) {
        frame_buffer_ = new FrameBuffer(MIN(camera_width_, camera_height_),
                                        MAX(camera_width_, camera_height_),
                                        DEFAULT_VERTEX_MATRIX_SHADER, DEFAULT_OES_FRAGMENT_SHADER);

        enum RenderFrame frame_type = FIT;
        if (frame_type_ == 0) {
            frame_type = SQUARE;
        } else if (frame_type_ == 1) {
            frame_type = FIT;
        } else if (frame_type_ == 2) {
            frame_type = CROP;
        }
        render_screen_->SetFrame(MIN(camera_width_, camera_height_),
                                 MAX(camera_width_, camera_height_),
                                 screen_width_, screen_height_, frame_type);
    }
    if (start_recording) {
        start_recording = false;
        start_time_ = 0;
        encoder_->CreateEncoder(egl_core_, frame_buffer_->GetTextureId());
        encoding_ = true;
    }
    render_screen_->SetOutput(screen_width_, screen_height_);
    frame_buffer_->SetOutput(MIN(camera_width_, camera_height_),
            MAX(camera_width_, camera_height_));
    frame_buffer_->SetTextureType(TEXTURE_OES);
    frame_buffer_->ActiveProgram();
    int texture_id = frame_buffer_->OnDrawFrame(oes_texture_id_, texture_matrix_);
    if (nullptr != image_process_) {
        if (render_time_ == 0) {
            render_time_ = getCurrentTime();
        }
        auto current_time = getCurrentTime() - render_time_;
        texture_id = image_process_->Process(texture_id, current_time,
                MIN(camera_width_, camera_height_),
                MAX(camera_width_, camera_height_), 0, 0);
    }
    int third_party_id = OnDrawFrame(texture_id,
                                 camera_width_, camera_height_);
    if (third_party_id > 0) {
        texture_id = third_party_id;
    }
    render_screen_->ActiveProgram();
    render_screen_->ProcessImage(texture_id);
    if (!egl_core_->SwapBuffers(preview_surface_)) {
        LOGE("eglSwapBuffers(preview_surface_) returned error %d", eglGetError());
    }
}

void CameraRecord::ConfigCamera() {
    LOGI("enter ConfigCamera");
    JNIEnv *env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return;
    }
    if (env == nullptr) {
        LOGE("getJNIEnv failed");
        return;
    }
    jclass clazz = env->GetObjectClass(obj_);
    if (nullptr != clazz) {
        jmethodID preview_width = env->GetMethodID(clazz, "getCameraWidth", "()I");
        if (nullptr != preview_width) {
            int width = env->CallIntMethod(obj_, preview_width);
            if (width != camera_width_) {
                camera_size_change_ = true;
            }
            this->camera_width_ = width;
        }

        jmethodID preview_height = env->GetMethodID(clazz, "getCameraHeight", "()I");
        if (nullptr != preview_height) {
            int height = env->CallIntMethod(obj_, preview_height);
            if (height != camera_height_) {
                camera_size_change_ = true;
            }
            this->camera_height_ = height;
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
    JNIEnv *env = nullptr;
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
        if (nullptr != releaseCameraCallback) {
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
    auto *record = reinterpret_cast<CameraRecord*>(myself);
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

    if (nullptr != image_process_) {
        delete image_process_;
    }
    image_process_ = new ImageProcess();

    StartCameraPreview();
    ConfigCamera();
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
    if (nullptr != image_process_) {
        delete image_process_;
        image_process_ = nullptr;
    }

    DestroyPreviewSurface();
    ReleaseCamera();
    if (nullptr != egl_core_) {
        LOGI("destroy context: %p", egl_core_->GetContext());
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
        PostMessage(new Message(MSG_EGL_CREATE_PREVIEW_SURFACE));
    }
}

void CameraRecord::DestroyWindowSurface() {
    PostMessage(new Message(MSG_EGL_DESTROY_PREVIEW_SURFACE));
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
    int video_width = static_cast<int>((floor(width / 16.0f)) * 16);
    int video_height = static_cast<int>((floor(height / 16.0f)) * 16);
    if (nullptr != packet_thread_) {
        PacketPool::GetInstance()->InitRecordingVideoPacketQueue();
        PacketPool::GetInstance()->InitAudioPacketQueue(44100);
        AudioPacketPool::GetInstance()->InitAudioPacketQueue();
        int ret = packet_thread_->Init(path, video_width, video_height, frame_rate,
             video_bit_rate * 1000, audio_sample_rate, audio_channel, audio_bit_rate * 1000, "libfdk_aac");
        if (ret >= 0) {
            packet_thread_->StartAsync();
        }
    }
    if (use_hard_encode) {
        encoder_ = new MediaEncodeAdapter(vm_, obj_);
    } else {
        encoder_ = new SoftEncoderAdapter(vertex_coordinate_, texture_coordinate_);
    }
    encoder_->Init(video_width, video_height, video_bit_rate * 1000, frame_rate);
    PostMessage(new Message(MSG_START_RECORDING));
}

void CameraRecord::StartRecording() {
    LOGI("StartRecording");
    start_recording = true;
}

void CameraRecord::StopEncoding() {
    LOGI("StopEncoding");
    PostMessage(new Message(MSG_STOP_RECORDING));
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
            FPS();
        }

        int64_t duration = getCurrentTime() - start_time_;
        if (encoding_ && nullptr != encoder_) {
            encoder_->Encode(static_cast<int>(duration * speed_));
        }
    }
}

void CameraRecord::SetFrameType(int frame) {
    frame_type_ = frame;
}

void CameraRecord::FPS() {
    if (pre_fps_count_time_ == 0) {
        pre_fps_count_time_ = getCurrentTime();
    }
    frame_count_ += 1;
    long current_time = getCurrentTime();
    if (current_time > pre_fps_count_time_ + 1000) {
        fps_ = frame_count_ * 1.0F * 1000 / (current_time - pre_fps_count_time_);
        frame_count_ = 0;
        pre_fps_count_time_ = current_time;
    }

//    LOGD("fps: %f", fps_);
}

int CameraRecord::AddFilter(const char *config_path) {
    int action_id = current_action_id_++;
    size_t len = strlen(config_path) + 1;
    char* path = new char[len];
    memcpy(path, config_path, len);
    auto* message = new Message(kFilter, action_id, 0, path);
    PostMessage(message);
    return action_id;
}

void CameraRecord::UpdateFilter(const char *config_path, int start_time, int end_time, int action_id) {
    size_t len = strlen(config_path) + 1;
    char* path = new char[len];
    memcpy(path, config_path, len);
    auto* message = new Message(kFilterUpdate, action_id, start_time, end_time, path);
    PostMessage(message);
}

void CameraRecord::DeleteFilter(int action_id) {
    auto* message = new Message(kFilterDelete, action_id, 0);
    PostMessage(message);
}

int CameraRecord::AddAction(const char *effect_config) {

}

void CameraRecord::UpdateAction(int start_time, int end_time, int action_id) {

}

void CameraRecord::DeleteAction(int action_id) {

}

void CameraRecord::OnAddFilter(char* config_path, int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("enter %s id: %d config: %s", __func__, action_id, config_path);
    image_process_->OnFilter(config_path, action_id);
    delete[] config_path;
    LOGI("leave %s", __func__);
}

void CameraRecord::OnUpdateFilter(char *config_path, int action_id,
        int start_time, int end_time) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("enter %s config_path: %s action_id: %d start_time: %d end_time: %d", __func__, config_path, action_id, start_time, end_time); // NOLINT
    image_process_->OnFilter(config_path, action_id, start_time, end_time);
    delete[] config_path;
    LOGI("leave %s", __func__);
}

void CameraRecord::OnDeleteFilter(int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("enter %s action_id: %d", __func__, action_id);
    image_process_->OnDeleteFilter(action_id);
    LOGI("leave %s", __func__);
}

void CameraRecord::HandleMessage(trinity::Message *msg) {
    int what = msg->GetWhat();
    switch (what) {
        case MSG_EGL_THREAD_CREATE:
            Initialize();
            break;
        case MSG_EGL_CREATE_PREVIEW_SURFACE:
            CreatePreviewSurface();
            break;
        case MSG_SWITCH_CAMERA_FACING:
            SwitchCamera();
            break;
        case MSG_START_RECORDING:
            StartRecording();
            break;
        case MSG_STOP_RECORDING:
            StopRecording();
            break;
        case MSG_EGL_DESTROY_PREVIEW_SURFACE:
            DestroyPreviewSurface();
            break;
        case MSG_EGL_THREAD_EXIT:
            Destroy();
            break;
        case MSG_RENDER_FRAME:
            RenderFrame();
            break;
        case MSG_SET_FRAME:
            SetFrameType(msg->GetArg1());
            break;
        case kFilter:
            OnAddFilter(reinterpret_cast<char*>(msg->GetObj()), msg->GetArg1());
            break;
        case kFilterUpdate:
            OnUpdateFilter(reinterpret_cast<char*>(msg->GetObj()), msg->GetArg1(), msg->GetArg2(), msg->GetArg3());
            break;
        case kFilterDelete:
            OnDeleteFilter(msg->GetArg1());
            break;
        default:
            break;
    }
}

}  // namespace trinity
