//
// Created by wlanjie on 2019-06-15.
//

#include "error_code.h"
#include "video_export.h"
#include "soft_encoder_adapter.h"
#include "android_xlog.h"
#include "tools.h"
#include <math.h>

namespace trinity {

VideoExport::VideoExport() {
    export_ing = false;
    egl_core_ = nullptr;
    egl_surface_ = EGL_NO_SURFACE;
    video_state_ = nullptr;
    yuv_render_ = nullptr;
    export_index_ = 0;
    video_width_ = 0;
    video_height_ = 0;
    frame_width_ = 0;
    frame_height_ = 0;
    encoder_ = nullptr;
    packet_thread_ = nullptr;
}

VideoExport::~VideoExport() {

}

static int OnCompleteState(StateContext *context) {
    VideoExport* editor = (VideoExport*) context->context;
    return editor->OnComplete();
}

int VideoExport::Export(deque<MediaClip*> clips, const char *path, int width, int height, int frame_rate, int video_bit_rate,
        int sample_rate, int channel_count, int audio_bit_rate) {
    if (clips.empty()) {
        return CLIP_EMPTY;
    }
    export_ing = true;
    packet_thread_ = new VideoConsumerThread();
    int ret = packet_thread_->Init(path, width, height, frame_rate, video_bit_rate, 0, channel_count, audio_bit_rate, "libfdk_aac");
    if (ret < 0) {
        return ret;
    }
    PacketPool::GetInstance()->InitRecordingVideoPacketQueue();
    PacketPool::GetInstance()->InitAudioPacketQueue(44100);
    AudioPacketPool::GetInstance()->InitAudioPacketQueue();
    packet_thread_->StartAsync();

    video_width_ = width;
    video_height_ = height;
    // copy clips to export clips
    for (int i = 0; i < clips.size(); ++i) {
        MediaClip* clip = clips.at(i);
        MediaClip* export_clip = new MediaClip();
        export_clip->start_time = clip->start_time;
        export_clip->end_time  = clip->end_time;
        export_clip->file_name = clip->file_name;
        clip_deque_.push_back(export_clip);
    }
    encoder_ = new SoftEncoderAdapter(0);
    encoder_->Init(width, height, video_bit_rate, frame_rate);
    MediaClip* clip = clip_deque_.at(0);
    video_state_ = static_cast<VideoState*>(av_malloc(sizeof(VideoState)));

    StateContext* state_context = (StateContext*) av_malloc(sizeof(StateContext));
    state_context->complete = OnCompleteState;
    state_context->context = this;
    video_state_->state_context = state_context;

    video_state_->start_time = 0;
    video_state_->end_time = INT64_MAX;
    av_start(video_state_, clip->file_name);
    pthread_create(&export_thread_, nullptr, ExportThread, this);
    return 0;
}

int VideoExport::OnComplete() {
    export_ing = false;
    return 0;
}

void* VideoExport::ExportThread(void* context) {
    VideoExport* video_export = (VideoExport*) (context);
    video_export->ProcessExport();
    pthread_exit(0);
}

void VideoExport::ProcessExport() {
    egl_core_ = new EGLCore();
    egl_core_->InitWithSharedContext();
    egl_surface_ = egl_core_->CreateOffscreenSurface(64, 64);
    if (nullptr == egl_surface_ || EGL_NO_SURFACE == egl_surface_) {
        return;
    }
    egl_core_->MakeCurrent(egl_surface_);
    FrameBuffer* frame_buffer = new FrameBuffer(video_width_, video_height_, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    image_process_ = new ImageProcess();
    encoder_->CreateEncoder(egl_core_, frame_buffer->GetTextureId());
    while (export_ing) {
        if (video_state_->video_queue.size == 0) {
            av_usleep(10000);
            continue;
        }
        Frame* frame = frame_queue_peek_last(&video_state_->video_queue);
        if (frame->frame != nullptr) {
            if (isnan(frame->pts)) {
                break;
            }
            if (!frame->uploaded && frame->frame != nullptr) {
                int width = MIN(frame->frame->linesize[0], frame->frame->width);
                int height = frame->frame->height;
                if (frame_width_ != width || frame_height_ != height) {
                    frame_width_ = width;
                    frame_height_ = height;
                    if (nullptr != yuv_render_) {
                        delete yuv_render_;
                    }
                    yuv_render_ = new YuvRender(frame->width, frame->height, 0);
                }
                int texture_id = yuv_render_->DrawFrame(frame->frame);
                uint64_t current_time = (uint64_t) (frame->frame->pts * av_q2d(video_state_->video_st->time_base) *
                                                    1000);
                texture_id = image_process_->Process(texture_id, current_time, frame->width, frame->height, 0, 0);
                frame_buffer->OnDrawFrame(texture_id);
                if (!egl_core_->SwapBuffers(egl_surface_)) {
                    LOGE("eglSwapBuffers error: %d", eglGetError());
                }
                encoder_->Encode();
                frame->uploaded = 1;
            }
        }
    }
    encoder_->DestroyEncoder();
    delete encoder_;
    LOGE("export done");
    packet_thread_->Stop();
    PacketPool::GetInstance()->DestroyRecordingVideoPacketQueue();
    PacketPool::GetInstance()->DestroyAudioPacketQueue();
    AudioPacketPool::GetInstance()->DestroyAudioPacketQueue();
    delete packet_thread_;

    egl_core_->ReleaseSurface(egl_surface_);
    egl_core_->Release();
    egl_surface_ = EGL_NO_SURFACE;
    delete egl_core_;
}

}