//
// Created by wlanjie on 2019-06-29.
//

#ifndef TRINITY_VIDEO_PLAYER_H
#define TRINITY_VIDEO_PLAYER_H

extern "C" {
#include "ffmpeg_decode.h"
};

namespace trinity {

typedef struct {
    double speed;
    int paused;
    int *queue_serial;
    double pts;
    double last_update;
    double pts_drift;
    int serial;
} Clock;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct {
    void (*render_video_frame)(struct VideoRenderContext* render);
    void* context;
} VideoRenderContext;

typedef struct {
    double frame_timer;
    int force_refresh;
    double last_vis_time;
    int av_sync_type;
    Clock video_clock;
    Clock sample_clock;
    Clock external_clock;
} PlayerState;

class VideoPlayer {

public:
    VideoPlayer();
    virtual ~VideoPlayer();
    int Start(const char* file_name, VideoRenderContext* render_context);
    void Stop();
private:
    static void* SyncThread(void* arg);
    void Sync();

private:
    pthread_t sync_thread_;
    pthread_mutex_t sync_mutex_;
    pthread_cond_t sync_cond_;

    VideoRenderContext* video_render_context_;
    MediaDecode* media_decode_;
    PlayerState* player_state_;
};

}

#endif //TRINITY_VIDEO_PLAYER_H
