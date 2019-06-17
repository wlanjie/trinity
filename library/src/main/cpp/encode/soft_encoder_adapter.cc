#include "soft_encoder_adapter.h"
#include "android_xlog.h"
#include "tools.h"

namespace trinity {

SoftEncoderAdapter::SoftEncoderAdapter(int strategy) :
        msg_(MSG_NONE), copy_texture_surface_(EGL_NO_SURFACE) {
    yuy_packet_pool_ = nullptr;
    load_texture_context_ = EGL_NO_CONTEXT;
    fbo_ = 0;
    output_texture_id_ = 0;
    egl_core_ = nullptr;
    encode_render_ = nullptr;
    pixel_size_ = 0;
    encoder_ = nullptr;
    renderer_ = nullptr;
    strategy_ = strategy;

    pthread_mutex_init(&lock_, NULL);
    pthread_cond_init(&condition_, NULL);
    pthread_mutex_init(&preview_thread_lock_, NULL);
    pthread_cond_init(&preview_thread_condition_, NULL);
}

SoftEncoderAdapter::~SoftEncoderAdapter() {
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&condition_);
    pthread_mutex_destroy(&preview_thread_lock_);
    pthread_cond_destroy(&preview_thread_condition_);
}

void SoftEncoderAdapter::CreateEncoder(EGLCore *eglCore, int inputTexId) {
    LOGI("enter CreateEncoder");
    this->load_texture_context_ = eglCore->GetContext();
    this->texture_id_ = inputTexId;
    pixel_size_ = video_width_ * video_height_ * PIXEL_BYTE_SIZE;

    start_time_ = 0;
    fps_change_time_ = 0;

    encoder_ = new VideoX264Encoder(strategy_);
    encoder_->Init(video_width_, video_height_, video_bit_rate_, frame_rate_, packet_pool_);
    yuy_packet_pool_ = new VideoPacketQueue();
    pthread_create(&x264_encoder_thread_, NULL, StartEncodeThread, this);
    msg_ = MSG_WINDOW_SET;
    pthread_create(&image_download_thread_, NULL, StartDownloadThread, this);

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

void SoftEncoderAdapter::Encode() {
    while (msg_ == MSG_WINDOW_SET || NULL == egl_core_) {
        usleep(100 * 1000);
    }
    if (start_time_ == 0)
        start_time_ = getCurrentTime();

    if (fps_change_time_ == 0){
        fps_change_time_ = getCurrentTime();
    }

    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fps_change_time_) / 1000.0f * frame_rate_ + 0.5f);
    if (expectedFrameCount < encode_frame_count_) {
        LOGE("expectedFrameCount is %d while encoded_frame_count_ is %d", expectedFrameCount,
             encode_frame_count_);
        return;
    }
    encode_frame_count_++;
    pthread_mutex_lock(&preview_thread_lock_);
    pthread_mutex_lock(&lock_);
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    pthread_cond_wait(&preview_thread_condition_, &preview_thread_lock_);
    pthread_mutex_unlock(&preview_thread_lock_);
}

void SoftEncoderAdapter::DestroyEncoder() {
    yuy_packet_pool_->Abort();
    pthread_join(x264_encoder_thread_, 0);
    delete yuy_packet_pool_;
    yuy_packet_pool_ = nullptr;
    if (NULL != encoder_) {
        encoder_->Destroy();
        delete encoder_;
        encoder_ = NULL;
    }

    pthread_mutex_lock(&lock_);
    msg_ = MSG_RENDER_LOOP_EXIT;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    pthread_join(image_download_thread_, 0);
}

void *SoftEncoderAdapter::StartDownloadThread(void *ptr) {
    SoftEncoderAdapter *softEncoderAdapter = (SoftEncoderAdapter *) ptr;
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
        if (nullptr != egl_core_) {
            egl_core_->MakeCurrent(copy_texture_surface_);
            this->LoadTexture();
            pthread_cond_wait(&condition_, &lock_);
        }
        pthread_mutex_unlock(&lock_);
    }
    return;
}

void SoftEncoderAdapter::LoadTexture() {
    if (0 == start_time_) {
        return;
    }
    int recordingDuration = getCurrentTime() - start_time_;
    glViewport(0, 0, video_width_, video_height_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_texture_id_, 0);
    renderer_->ProcessImage(texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    this->SignalPreviewThread();
    uint8_t *packetBuffer = new uint8_t[pixel_size_];
    encode_render_->CopyYUV2Image(output_texture_id_, packetBuffer, video_width_, video_height_);
    VideoPacket *videoPacket = new VideoPacket();
    videoPacket->buffer = packetBuffer;
    videoPacket->size = pixel_size_;
    videoPacket->timeMills = recordingDuration;
    yuy_packet_pool_->Put(videoPacket);
}

void SoftEncoderAdapter::SignalPreviewThread() {
    pthread_mutex_lock(&preview_thread_lock_);
    pthread_cond_signal(&preview_thread_condition_);
    pthread_mutex_unlock(&preview_thread_lock_);
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
    glBindTexture(GL_TEXTURE_2D, output_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, video_width_, video_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    SoftEncoderAdapter *softEncoderAdapter = (SoftEncoderAdapter *) ptr;
    softEncoderAdapter->startEncode();
    pthread_exit(0);
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

}
