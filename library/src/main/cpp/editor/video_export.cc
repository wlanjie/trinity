//
// Created by wlanjie on 2019-06-15.
//

#include "error_code.h"
#include "video_export.h"
#include "soft_encoder_adapter.h"
#include "android_xlog.h"
#include "tools.h"
#include "error_code.h"
#include <math.h>

namespace trinity {

VideoExport::VideoExport() {
    export_ing = false;
    egl_core_ = nullptr;
    egl_surface_ = EGL_NO_SURFACE;
    media_decode_ = nullptr;
    state_event_ = nullptr;
    yuv_render_ = nullptr;
    export_index_ = 0;
    video_width_ = 0;
    video_height_ = 0;
    frame_width_ = 0;
    frame_height_ = 0;
    encoder_ = nullptr;
    packet_thread_ = nullptr;
    image_process_ = nullptr;
}

VideoExport::~VideoExport() {

}

int VideoExport::OnCompleteState(StateEvent *event) {
    VideoExport* video_export = (VideoExport*) event->context;
    return video_export->OnComplete();
}

int VideoExport::OnComplete() {
    export_ing = false;
    return 0;
}

int VideoExport::Export(const char *export_config, const char *path, int width, int height, int frame_rate,
                        int video_bit_rate, int sample_rate, int channel_count, int audio_bit_rate) {
    cJSON* json = cJSON_Parse(export_config);
    if (nullptr == json) {
        return EXPORT_CONFIG;
    }
    cJSON_Print(json);
    cJSON* clips = cJSON_GetObjectItem(json, "clips");
    if (nullptr == clips) {
        return CLIP_EMPTY;
    }
    int clip_size = cJSON_GetArraySize(clips);
    if (clip_size == 0) {
        return CLIP_EMPTY;
    }
    LOGE("Start Export");
    cJSON* item = clips->child;

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

    for (int i = 0; i < clip_size; i++) {
        cJSON* path_item = cJSON_GetObjectItem(item, "path");
        cJSON* start_time_item = cJSON_GetObjectItem(item, "startTime");
        cJSON* end_time_item = cJSON_GetObjectItem(item, "endTime");
        item = item->next;

        MediaClip* export_clip = new MediaClip();
        export_clip->start_time = start_time_item->valueint;
        export_clip->end_time  = end_time_item->valueint;
        export_clip->file_name = path_item->valuestring;
        clip_deque_.push_back(export_clip);
        LOGE("path: %s start_time: %d end_time: %d", path_item->valuestring, start_time_item->valueint, end_time_item->valueint);
    }

    cJSON* effects = cJSON_GetObjectItem(json, "effects");
    if (nullptr != effects) {
        int effect_size = cJSON_GetArraySize(effects);
        if (clip_size > 0) {
            cJSON* effects_child = effects->child;
            image_process_ = new ImageProcess();
            for (int i = 0; i < effect_size; ++i) {
                cJSON* type_item = cJSON_GetObjectItem(effects_child, "type");
                cJSON* start_time_item = cJSON_GetObjectItem(effects_child, "start_time");
                cJSON* end_time_item = cJSON_GetObjectItem(effects_child, "end_time");
                cJSON* split_count_item = cJSON_GetObjectItem(effects_child, "split_screen_count");
                if (type_item->valueint == SPLIT_SCREEN) {
                    image_process_->AddSplitScreenAction(split_count_item->valueint, start_time_item->valueint, end_time_item->valueint);
                }
                effects_child = effects_child->next;
            }
        }
    }

    encoder_ = new SoftEncoderAdapter(0);
    encoder_->Init(width, height, video_bit_rate, frame_rate);
    MediaClip* clip = clip_deque_.at(0);

    media_decode_ = (MediaDecode*) av_malloc(sizeof(MediaDecode));
    memset(media_decode_, 0, sizeof(MediaDecode));
    media_decode_->start_time = clip->start_time;
    media_decode_->end_time = clip->end_time == 0 ? INT64_MAX : clip->end_time;

    state_event_ = (StateEvent*) av_malloc(sizeof(StateEvent));
    memset(state_event_, 0, sizeof(StateEvent));
    state_event_->context = this;
    state_event_->on_complete_event = OnCompleteState;
    media_decode_->state_event = state_event_;

    av_decode_start(media_decode_, clip->file_name);
    pthread_create(&export_thread_, nullptr, ExportThread, this);
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
    encoder_->CreateEncoder(egl_core_, frame_buffer->GetTextureId());
    while (export_ing) {
        if (media_decode_->video_frame_queue.size == 0) {
            av_usleep(10000);
            continue;
        }
        if (frame_queue_nb_remaining(&media_decode_->video_frame_queue) == 0) {
            continue;
        }
        Frame* vp = frame_queue_peek(&media_decode_->video_frame_queue);
        if (vp->serial != media_decode_->video_packet_queue.serial) {
            frame_queue_next(&media_decode_->video_frame_queue);
            continue;
        }
        frame_queue_next(&media_decode_->video_frame_queue);
        Frame* frame = frame_queue_peek_last(&media_decode_->video_frame_queue);
        if (frame->frame != nullptr) {
            if (isnan(frame->pts)) {
//                break;
                LOGE("isnan");
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
                    yuv_render_ = new YuvRender(frame->width, frame->height, 180);
                }
                int texture_id = yuv_render_->DrawFrame(frame->frame);
                uint64_t current_time = (uint64_t) (frame->frame->pts * av_q2d(media_decode_->video_stream->time_base) * 1000);
                if (image_process_ != nullptr) {
                    texture_id = image_process_->Process(texture_id, current_time, frame->width, frame->height, 0, 0);
                }
                frame_buffer->OnDrawFrame(texture_id);
                if (!egl_core_->SwapBuffers(egl_surface_)) {
                    LOGE("eglSwapBuffers error: %d", eglGetError());
                }
                encoder_->Encode(current_time);
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