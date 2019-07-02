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

#include "video_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "yuv_render.h"
#include "image_process.h"

namespace trinity {

using namespace std;

typedef struct {
    char* file_name;
    int64_t start_time;
    int64_t end_time;
} MediaClip;

class VideoExport {
public:
    VideoExport();
    ~VideoExport();

    int Export(const char* export_config, const char* path,
            int width, int height, int frame_rate, int video_bit_rate,
            int sample_rate, int channel_count, int audio_bit_rate);

private:
    static void* ExportThread(void* context);
    void ProcessExport();
    static int OnCompleteState(StateEvent* event);
    int OnComplete();

private:
    pthread_t export_thread_;
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
    VideoConsumerThread* packet_thread_;
};

}

#endif //TRINITY_VIDEO_EXPORT_H
