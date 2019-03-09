#include "./soft_encoder_adapter.h"

#define LOG_TAG "SoftEncoderAdapter"

SoftEncoderAdapter::SoftEncoderAdapter(int strategy) :
        msg_(MSG_NONE), copy_texture_surface_(0) {
    host_gpu_copier_ = NULL;
    egl_core_ = NULL;
    encoder_ = NULL;
    output_texture_id_ = -1;
    this->strategy_ = strategy;

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
    host_gpu_copier_ = new HostGPUCopier();

    start_time_ = -1;
    fps_change_time_ = -1;

    encoder_ = new VideoX264Encoder(strategy_);
    encoder_->Init(video_width_, video_height_, video_bit_rate_, frameRate, packet_pool_);
    yuy_packet_pool_ = LiveYUY2PacketPool::GetInstance();
    yuy_packet_pool_->InitYUY2PacketQueue();
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
    if (start_time_ == -1)
        start_time_ = getCurrentTime();

    if (fps_change_time_ == -1){
        fps_change_time_ = getCurrentTime();
    }

    int64_t curTime = getCurrentTime() - start_time_;
    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fps_change_time_) / 1000.0f * frameRate + 0.5f);
    if (expectedFrameCount < encoded_frame_count_) {
        LOGI("expectedFrameCount is %d while encoded_frame_count_ is %d", expectedFrameCount,
             encoded_frame_count_);
        return;
    }
    encoded_frame_count_++;
    pthread_mutex_lock(&preview_thread_lock_);
    pthread_mutex_lock(&lock_);
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    pthread_cond_wait(&preview_thread_condition_, &preview_thread_lock_);
    pthread_mutex_unlock(&preview_thread_lock_);
}

void SoftEncoderAdapter::DestroyEncoder() {
    yuy_packet_pool_->AbortYUY2PacketQueue();
    pthread_join(x264_encoder_thread_, 0);
    yuy_packet_pool_->DestroyYUY2PacketQueue();
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
    if (NULL != host_gpu_copier_) {
        host_gpu_copier_->Destroy();
    }
}

void *SoftEncoderAdapter::StartDownloadThread(void *ptr) {
    SoftEncoderAdapter *softEncoderAdapter = (SoftEncoderAdapter *) ptr;
    softEncoderAdapter->renderLoop();
    pthread_exit(0);
    return 0;
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
        if (NULL != egl_core_) {
            egl_core_->MakeCurrent(copy_texture_surface_);
            this->LoadTexture();
            pthread_cond_wait(&condition_, &lock_);
        }
        pthread_mutex_unlock(&lock_);
    }
    return;
}

void SoftEncoderAdapter::LoadTexture() {
    if (-1 == start_time_) {
        return;
    }
    //1:拷贝纹理到我们的临时纹理
    int recordingDuration = getCurrentTime() - start_time_;
    glViewport(0, 0, video_width_, video_height_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    checkGlError("glBindFramebuffer fbo_");
    long startTimeMills = getCurrentTime();
    renderer_->RenderToTexture(texture_id_, output_texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	LOGI("copy TexId waste timeMils 【%d】", (int)(getCurrentTime() - start_time_mills_));
    this->SignalPreviewThread();
    //2:从显存Download到内存
    startTimeMills = getCurrentTime();
    byte *packetBuffer = new byte[pixel_size_];
    host_gpu_copier_->CopyYUY2Image(output_texture_id_, packetBuffer, video_width_, video_height_);
//	LOGI("Download Texture waste timeMils 【%d】", (int)(getCurrentTime() - start_time_mills_));
    //3:构造LiveVideoPacket放到YUY2PacketPool里面
    LiveVideoPacket *videoPacket = new LiveVideoPacket();
    videoPacket->buffer = packetBuffer;
    videoPacket->size = pixel_size_;
    videoPacket->timeMills = recordingDuration;
//	LOGI("recordingDuration 【%d】", recordingDuration);
    yuy_packet_pool_->PushYUY2PacketToQueue(videoPacket);
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
    renderer_ = new VideoGLSurfaceRender();
    renderer_->Init(video_width_, video_height_);
    glGenFramebuffers(1, &fbo_);
    //初始化outputTexId
    glGenTextures(1, &output_texture_id_);
    glBindTexture(GL_TEXTURE_2D, output_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, video_width_, video_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void SoftEncoderAdapter::Destroy() {
    if (NULL != egl_core_) {
        egl_core_->MakeCurrent(copy_texture_surface_);
        if (fbo_) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo_);
        }
        egl_core_->ReleaseSurface(copy_texture_surface_);
        if (renderer_) {
            LOGI("delete renderer_..");
            renderer_->Destroy();
            delete renderer_;
            renderer_ = NULL;
        }
        egl_core_->Release();
        egl_core_ = NULL;
    }
}

void *SoftEncoderAdapter::StartEncodeThread(void *ptr) {
    SoftEncoderAdapter *softEncoderAdapter = (SoftEncoderAdapter *) ptr;
    softEncoderAdapter->startEncode();
    pthread_exit(0);
    return 0;
}

void SoftEncoderAdapter::startEncode() {
    LiveVideoPacket *videoPacket = NULL;
    while (true) {
//		LOGI("getYUY2PacketQueueSize is %d", yuy_packet_pool_->GetYUY2PacketQueueSize());
        if (yuy_packet_pool_->GetYUY2Packet(&videoPacket, true) < 0) {
            LOGI("yuy_packet_pool_->GetRecordingVideoPacket return negetive value...");
            break;
        }
        if (videoPacket) {
            //调用编码器编码这一帧数据
            encoder_->Encode(videoPacket);
            delete videoPacket;
            videoPacket = NULL;
        }
    }
}
