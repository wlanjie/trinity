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

namespace trinity {

SoftEncoderAdapter::SoftEncoderAdapter(GLfloat* vertex_coordinate, GLfloat* texture_coordinate)
    : yuy_packet_pool_(nullptr)
    , encoding_(true)
    , queue_()
    , vertex_coordinate_(nullptr)
    , texture_coordinate_(nullptr)
    , fbo_(0)
    , output_texture_id_(0)
    , core_(nullptr)
    , encode_render_(nullptr)
    , pixel_size_(0)
    , encoder_(nullptr)
    , renderer_(nullptr)
    , encoder_surface_(EGL_NO_SURFACE) {

    pthread_mutex_init(&packet_mutex_, nullptr);
    pthread_cond_init(&packet_cond_, nullptr);

    queue_ = new MessageQueue("X264Encode message queue");
    InitMessageQueue(queue_);
    pthread_create(&encoder_thread_, nullptr, MessageQueueThread, this);

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
    PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(encoder_thread_, nullptr);
    if (nullptr != queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = nullptr;
    }
    if (nullptr != vertex_coordinate_) {
        delete[] vertex_coordinate_;
        vertex_coordinate_ = nullptr;
    }
    if (nullptr != texture_coordinate_) {
        delete[] texture_coordinate_;
        texture_coordinate_ = nullptr;
    }
    pthread_mutex_destroy(&packet_mutex_);
    pthread_cond_destroy(&packet_cond_);
}

void* SoftEncoderAdapter::MessageQueueThread(void *args) {
    auto adapter = reinterpret_cast<SoftEncoderAdapter*>(args);
    adapter->EncodeLoop();
    pthread_exit(0);
}

void SoftEncoderAdapter::HandleMessage(trinity::Message *msg) {
    int what = msg->GetWhat();
    switch (what) {
        case kStartEncoder:
            CreateX264Encoder(reinterpret_cast<EGLCore*>(msg->GetObj()));
            break;

        case kEncodeFrame:
            EncodeFrame(msg->GetArg1(), msg->GetArg2());
            break;

        case kDestroyEncoder:
            DestroyX264Encoder();
            break;

        default:
            break;
    }
}

void SoftEncoderAdapter::CreateX264Encoder(EGLCore* core) {
    core_ = new EGLCore();
    core_->Init(core->GetContext());
    encoder_surface_ = core_->CreateOffscreenSurface(video_width_, video_height_);
    core_->MakeCurrent(encoder_surface_);

    pixel_size_ = video_width_ * video_height_ * 3 / 2;
    start_time_ = 0;
    fps_change_time_ = 0;
    encoder_ = new VideoX264Encoder(0);
    encoder_->Init(video_width_, video_height_, video_bit_rate_, frame_rate_, packet_pool_);
    yuy_packet_pool_ = new VideoPacketQueue();
    Initialize();
    pthread_create(&x264_encoder_thread_, nullptr, StartEncodeThread, this);
}

void SoftEncoderAdapter::EncodeFrame(int texture_id, int time) {
    if (start_time_ == 0)
        start_time_ = getCurrentTime();

    if (fps_change_time_ == 0) {
        fps_change_time_ = getCurrentTime();
    }

    // need drop frames
//    int64_t current_time = static_cast<int64_t>((getCurrentTime() - fps_change_time_) * speed);
    int expectedFrameCount = static_cast<int>(time / 1000.0F * frame_rate_ + 0.5F);
    if (expectedFrameCount < encode_frame_count_) {
        LOGE("expectedFrameCount is %d while encoded_frame_count_ is %d", expectedFrameCount,
             encode_frame_count_);
        pthread_mutex_lock(&packet_mutex_);
        pthread_cond_signal(&packet_cond_);
        pthread_mutex_unlock(&packet_mutex_);
        return;
    }
    EncodeTexture(static_cast<GLuint>(texture_id), time);
    encode_frame_count_++;
}

void SoftEncoderAdapter::DestroyX264Encoder() {
    LOGE("enter: %s", __func__);
    encoding_ = false;
    pthread_join(x264_encoder_thread_, 0);
    yuy_packet_pool_->Abort();
    delete yuy_packet_pool_;
    yuy_packet_pool_ = nullptr;

    if (nullptr != core_ && EGL_NO_SURFACE != encoder_surface_) {
        core_->MakeCurrent(encoder_surface_);
        if (nullptr != encode_render_) {
            encode_render_->Destroy();
            delete encode_render_;
            encode_render_ = nullptr;
        }
        if (output_texture_id_ != 0) {
            glDeleteTextures(1, &output_texture_id_);
            output_texture_id_ = 0;
        }
        if (fbo_ != 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo_);
            fbo_ = 0;
        }
        if (nullptr != renderer_) {
            delete renderer_;
            renderer_ = nullptr;
        }
        core_->ReleaseSurface(encoder_surface_);
        delete core_;
        core_ = nullptr;
    }

    if (nullptr != encoder_) {
        encoder_->Destroy();
        delete encoder_;
        encoder_ = nullptr;
    }
}

void SoftEncoderAdapter::EncodeLoop() {
    while (true) {
        Message *msg = nullptr;
        if (queue_->DequeueMessage(&msg, true) > 0) {
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                break;
            }
            delete msg;
        }
    }
    LOGI("X264 Encode Thread ending...");
}

void SoftEncoderAdapter::CreateEncoder(EGLCore *core) {
    LOGI("enter CreateEncoder");
    PostMessage(new Message(kStartEncoder, core));
    LOGI("leave CreateEncoder");
}

void SoftEncoderAdapter::Encode(uint64_t time, int texture_id) {
    PostMessage(new Message(kEncodeFrame, texture_id, static_cast<int>(time)));
    pthread_mutex_lock(&packet_mutex_);
    pthread_cond_wait(&packet_cond_, &packet_mutex_);
    pthread_mutex_unlock(&packet_mutex_);
}

void SoftEncoderAdapter::DestroyEncoder() {
    LOGI("enter: %s", __func__);
    PostMessage(new Message(kDestroyEncoder));
}

void SoftEncoderAdapter::EncodeTexture(GLuint texture_id, int time) {
//    int recordingDuration = getCurrentTime() - start_time_;
//    glViewport(0, 0, video_width_, video_height_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    if (nullptr != vertex_coordinate_ && nullptr != texture_coordinate_) {
        renderer_->ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_);
    } else {
        renderer_->ProcessImage(texture_id);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // TODO 这里需要设置一个buffer池
    auto *packetBuffer = new uint8_t[pixel_size_];
    encode_render_->CopyYUV420Image(output_texture_id_, packetBuffer, video_width_, video_height_);
    auto *videoPacket = new VideoPacket();
    videoPacket->buffer = packetBuffer;
    videoPacket->size = pixel_size_;
    videoPacket->timeMills = time;
    if (nullptr != yuy_packet_pool_) {
        yuy_packet_pool_->Put(videoPacket);
    }
    pthread_mutex_lock(&packet_mutex_);
    pthread_cond_signal(&packet_cond_);
    pthread_mutex_unlock(&packet_mutex_);
}

bool SoftEncoderAdapter::Initialize() {
    renderer_ = new OpenGL(video_width_, video_height_);
    encode_render_ = new EncodeRender();
    glGenFramebuffers(1, &fbo_);
    // 初始化outputTexId
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

void *SoftEncoderAdapter::StartEncodeThread(void* args) {
    auto softEncoderAdapter = reinterpret_cast<SoftEncoderAdapter*>(args);
    softEncoderAdapter->StartEncode();
    pthread_exit(0);
}

void SoftEncoderAdapter::StartEncode() {
    VideoPacket *videoPacket = nullptr;
    while (true) {
        if (yuy_packet_pool_->Size() <= 0 && !encoding_) {
            break;
        }
        if (yuy_packet_pool_->Size() <= 0) {
            usleep(10000);
            continue;
        }
        if (yuy_packet_pool_->Get(&videoPacket, true) < 0) {
            continue;
        }
        if (videoPacket) {
            encoder_->Encode(videoPacket);
            delete videoPacket;
            videoPacket = nullptr;
        }
    }
    LOGE("Encode done");
}

}  // namespace trinity
