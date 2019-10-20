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

#include "soft_encoder_adapter.h"
#include "android_xlog.h"
#include "tools.h"

#define NO_TIME_MILLS -1000

namespace trinity {

SoftEncoderAdapter::SoftEncoderAdapter(GLfloat* vertex_coordinate, GLfloat* texture_coordinate)
    : yuy_packet_pool_(nullptr),
      load_texture_context_(EGL_NO_CONTEXT),
      vertex_coordinate_(nullptr),
      texture_coordinate_(nullptr),
      fbo_(0),
      output_texture_id_(0),
      egl_core_(nullptr),
      encode_render_(nullptr),
      pixel_size_(0),
      encoder_(nullptr),
      renderer_(nullptr),
      time_mills_(0),
      msg_(MSG_NONE),
      copy_texture_surface_(EGL_NO_SURFACE) {
    pthread_mutex_init(&lock_, NULL);
    pthread_cond_init(&condition_, NULL);

    // 因为encoder_render时不能改变顶点和纹理坐标
    // 而glReadPixels读取的图像又是上下颠倒的
    // 所以这里显示的把纹理坐标做180度旋转
    // 从而保证从glReadPixels读取的数据不是上下颠倒的,而是正确的
    if (vertex_coordinate != nullptr) {
        vertex_coordinate_ = new GLfloat[8];
        memcpy(vertex_coordinate_, vertex_coordinate, sizeof(GLfloat) * 8);
    }
    if (texture_coordinate != nullptr) {
        texture_coordinate_ = new GLfloat[8];
        memcpy(texture_coordinate_, texture_coordinate, sizeof(GLfloat) * 8);
    }
}

SoftEncoderAdapter::~SoftEncoderAdapter() {
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&condition_);
    if (nullptr != vertex_coordinate_) {
        delete[] vertex_coordinate_;
        vertex_coordinate_ = nullptr;
    }
    if (nullptr != texture_coordinate_) {
        delete[] texture_coordinate_;
        texture_coordinate_ = nullptr;
    }
}

void SoftEncoderAdapter::CreateEncoder(EGLCore *eglCore, int inputTexId) {
    LOGI("enter CreateEncoder");
    this->load_texture_context_ = eglCore->GetContext();
    this->texture_id_ = inputTexId;
    pixel_size_ = video_width_ * video_height_ * 3 / 2;

    start_time_ = 0;
    fps_change_time_ = 0;

    encoder_ = new VideoX264Encoder(0);
    encoder_->Init(video_width_, video_height_, video_bit_rate_, frame_rate_, packet_pool_);
    yuy_packet_pool_ = new VideoPacketQueue();
    pthread_create(&x264_encoder_thread_, nullptr, StartEncodeThread, this);
    msg_ = MSG_WINDOW_SET;
    pthread_create(&image_download_thread_, nullptr, StartDownloadThread, this);

    LOGI("leave CreateEncoder");
}

void SoftEncoderAdapter::ReConfigure(int maxBitRate, int avgBitRate, int fps) {
    if (encoder_) {
        encoder_->ReConfigure(avgBitRate);
    }
}

void SoftEncoderAdapter::HotConfig(int maxBitrate, int avgBitrate, int fps) {
    if (encoder_) {
        encoder_->ReConfigure(maxBitrate);
    }

    ResetFpsStartTimeIfNeed(fps);
}

void SoftEncoderAdapter::Encode(int timeMills) {
    while (msg_ == MSG_WINDOW_SET || NULL == egl_core_) {
        usleep(100 * 1000);
    }
    if (start_time_ == 0)
        start_time_ = getCurrentTime();

    if (fps_change_time_ == 0) {
        fps_change_time_ = getCurrentTime();
    }

    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fps_change_time_) / 1000.0f * frame_rate_ + 0.5f);
//    if (expectedFrameCount < encode_frame_count_) {
//        LOGE("expectedFrameCount is %d while encoded_frame_count_ is %d", expectedFrameCount,
//             encode_frame_count_);
//        return;
//    }
    time_mills_ = timeMills;
    encode_frame_count_++;
    pthread_mutex_lock(&lock_);
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
}

void SoftEncoderAdapter::DestroyEncoder() {
    yuy_packet_pool_->Abort();
    pthread_join(x264_encoder_thread_, 0);
    delete yuy_packet_pool_;
    yuy_packet_pool_ = nullptr;
    if (nullptr != encoder_) {
        encoder_->Destroy();
        delete encoder_;
        encoder_ = nullptr;
    }

    pthread_mutex_lock(&lock_);
    msg_ = MSG_RENDER_LOOP_EXIT;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    pthread_join(image_download_thread_, 0);
}

void *SoftEncoderAdapter::StartDownloadThread(void *ptr) {
    auto *softEncoderAdapter = reinterpret_cast<SoftEncoderAdapter*>(ptr);
    softEncoderAdapter->renderLoop();
    pthread_exit(0);
}

void SoftEncoderAdapter::renderLoop() {
    bool renderingEnabled = true;
    while (renderingEnabled) {
        pthread_mutex_lock(&lock_);
        switch (msg_) {
            case MSG_WINDOW_SET:
                LOGI("receive msg MSG_WINDOW_SET");
                Initialize();
                break;
            case MSG_RENDER_LOOP_EXIT:
                LOGI("receive msg MSG_RENDER_LOOP_EXIT");
                renderingEnabled = false;
                Destroy();
                break;
            default:
                break;
        }
        msg_ = MSG_NONE;
        if (nullptr != egl_core_ && nullptr != yuy_packet_pool_) {
            egl_core_->MakeCurrent(copy_texture_surface_);
            this->LoadTexture();
            pthread_cond_wait(&condition_, &lock_);
        }
        pthread_mutex_unlock(&lock_);
    }
}

void SoftEncoderAdapter::LoadTexture() {
    if (0 == start_time_ || time_mills_ == NO_TIME_MILLS) {
        SignalPreviewThread();
        return;
    }
//    int recordingDuration = getCurrentTime() - start_time_;
//    glViewport(0, 0, video_width_, video_height_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    if (nullptr != vertex_coordinate_ && nullptr != texture_coordinate_) {
        renderer_->ProcessImage(texture_id_, vertex_coordinate_, texture_coordinate_);
    } else {
        renderer_->ProcessImage(texture_id_);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    this->SignalPreviewThread();
    // TODO 这里需要设置一个buffer池
    auto *packetBuffer = new uint8_t[pixel_size_];
    encode_render_->CopyYUV420Image(output_texture_id_, packetBuffer, video_width_, video_height_);
    auto *videoPacket = new VideoPacket();
    videoPacket->buffer = packetBuffer;
    videoPacket->size = pixel_size_;
    videoPacket->timeMills = time_mills_;
    if (time_mills_ != -1) {
        time_mills_ = NO_TIME_MILLS;
    }
    if (nullptr != yuy_packet_pool_) {
        yuy_packet_pool_->Put(videoPacket);
    }
}

void SoftEncoderAdapter::SignalPreviewThread() {
}

bool SoftEncoderAdapter::Initialize() {
    egl_core_ = new EGLCore();
    egl_core_->Init(load_texture_context_);
    copy_texture_surface_ = egl_core_->CreateOffscreenSurface(video_width_, video_height_);
    egl_core_->MakeCurrent(copy_texture_surface_);
    renderer_ = new OpenGL(video_width_, video_height_);
    encode_render_ = new EncodeRender();
    glGenFramebuffers(1, &fbo_);
    //初始化outputTexId
    glGenTextures(1, &output_texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glBindTexture(GL_TEXTURE_2D, output_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, video_width_, video_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_texture_id_, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void SoftEncoderAdapter::Destroy() {
    if (nullptr != egl_core_) {
        egl_core_->MakeCurrent(copy_texture_surface_);
        if (nullptr != encode_render_) {
            encode_render_->Destroy();
            delete encode_render_;
            encode_render_ = nullptr;
        }
        if (output_texture_id_ != 0) {
            glDeleteTextures(1, &output_texture_id_);
            output_texture_id_ = 0;
        }
        // TODO delete texture
        if (fbo_ != 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo_);
            fbo_ = 0;
        }
        egl_core_->ReleaseSurface(copy_texture_surface_);
        if (nullptr != renderer_) {
            delete renderer_;
            renderer_ = nullptr;
        }
        egl_core_->Release();
        delete egl_core_;
        egl_core_ = nullptr;
    }
}

void *SoftEncoderAdapter::StartEncodeThread(void *ptr) {
    auto *softEncoderAdapter = reinterpret_cast<SoftEncoderAdapter *>(ptr);
    softEncoderAdapter->startEncode();
    pthread_exit(nullptr);
}

void SoftEncoderAdapter::startEncode() {
    VideoPacket *videoPacket = nullptr;
    while (true) {
        if (yuy_packet_pool_->Get(&videoPacket, true) < 0) {
            LOGI("yuy_packet_pool_->GetRecordingVideoPacket return negetive value...");
            break;
        }
        if (videoPacket) {
            encoder_->Encode(videoPacket);
            delete videoPacket;
            videoPacket = nullptr;
        }
    }
}

}  // namespace trinity
