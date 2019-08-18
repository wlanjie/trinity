//
// Created by wlanjie on 2019-06-15.
//

#ifndef TRINITY_VIDEO_EXPORT_H
#define TRINITY_VIDEO_EXPORT_H

extern "C" {
#include "ffmpeg_decode.h"
#include "cJSON.h"
};

#include <deque>
#include <fstream>
#include <iostream>
#include <jni.h>

#include "video_encoder_adapter.h"
#include "audio_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "yuv_render.h"
#include "image_process.h"
#include "handler.h"
#include "trinity.h"

namespace trinity {

using namespace std;

enum {
    kStartNextExport
};

class VideoExportHandler;

class VideoExport {
public:
    VideoExport(JNIEnv* env, jobject object);
    ~VideoExport();

    int Export(const char* export_config, const char* path,
            int width, int height, int frame_rate, int video_bit_rate,
            int sample_rate, int channel_count, int audio_bit_rate);

    int OnComplete();
private:
    static void* ExportVideoThread(void* context);
    static int OnCompleteState(StateEvent* event);
    static void* ExportAudioThread(void* context);
    static void* ExportMessageThread(void* context);
    void StartDecode(MediaClip* clip);
    void FreeResource();
    void ProcessVideoExport();
    void ProcessAudioExport();
    void OnExportProgress(uint64_t current_time);
    void OnExportComplete();
    int Resample();
    void ProcessMessage();

private:
    JavaVM* vm_;
    jobject object_;
    int64_t video_duration_;
    pthread_t export_video_thread_;
    pthread_t export_audio_thread_;
    deque<MediaClip*> clip_deque_;
    bool export_ing;
    EGLCore* egl_core_;
    EGLSurface egl_surface_;
    MediaDecode* media_decode_;
    StateEvent* state_event_;
    int export_index_;
    int video_width_;
    int video_height_;
    int frame_width_;
    int frame_height_;
    YuvRender* yuv_render_;
    ImageProcess* image_process_;
    VideoEncoderAdapter* encoder_;
    AudioEncoderAdapter* audio_encoder_adapter_;
    VideoConsumerThread* packet_thread_;
    PacketPool* packet_pool_;
    uint64_t current_time_;
    uint64_t previous_time_;
    SwrContext* swr_context_;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    short* audio_samples_;
    VideoExportHandler* video_export_handler_;
    MessageQueue* video_export_message_queue_;
    pthread_t export_message_thread_;
    int time_diff_;
    pthread_mutex_t media_mutex_;
    pthread_cond_t media_cond_;
};

class VideoExportHandler : public Handler {
public:
    VideoExportHandler(VideoExport* video_export, MessageQueue* queue) : Handler(queue) {
        video_export_ = video_export;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        switch (what) {
            case kStartNextExport:
                video_export_->OnComplete();
                break;

            default:
                break;
        }
    }
private:
    VideoExport* video_export_;
};

}

#endif //TRINITY_VIDEO_EXPORT_H
