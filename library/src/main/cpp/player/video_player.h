//
// Created by wlanjie on 2019-06-29.
//

#ifndef TRINITY_VIDEO_PLAYER_H
#define TRINITY_VIDEO_PLAYER_H

#include "audio_render.h"
#include "handler.h"

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

typedef struct VideoEvent {
    void (*render_video_frame)(struct VideoEvent* event);
    void* context;
} VideoEvent;

typedef struct PlayerState {
    double frame_timer;
    int force_refresh;
    double last_vis_time;
    int av_sync_type;
    Clock video_clock;
    Clock sample_clock;
    Clock external_clock;

    double audio_clock;
    double audio_hw_buf_size;
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    double audio_diff_cum;
    unsigned int audio_buf_index;
    unsigned int audio_buf_size;
    unsigned int audio_buf1_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;

    SwrContext* swr_context;
    int step;
} PlayerState;

typedef enum {
    kNone = 0,
    kStartPlayer,
    // play state
    kPlaying,
    kResume,
    kPause,
    kStop
} PlayerMessage;

class PlayerHandler;

class VideoPlayer {

public:
    VideoPlayer();
    virtual ~VideoPlayer();
    int Start(const char* file_name, VideoEvent* video_event);
    void Stop();
    int ReadAudio(uint8_t* buffer, int buffer_size);
public:
    void StartPlayer();

private:
    static void* SyncThread(void* arg);
    void Sync();
    static void OnSeekEvent(SeekEvent* event, int seek_flag);
    static void OnAudioPrepareEvent(AudioEvent* event, int size);
    void StreamTogglePause(MediaDecode* media_decode, PlayerState* player_state);
private:
    pthread_t sync_thread_;
    pthread_mutex_t sync_mutex_;
    pthread_cond_t sync_cond_;

    VideoEvent* video_event_;
    MediaDecode* media_decode_;
    PlayerState* player_state_;

    AudioRender* audio_render_;
    PlayerHandler* player_handler_;
    MessageQueue* player_queue_;
};

class PlayerHandler : public Handler {
public:
    PlayerHandler(VideoPlayer* player, MessageQueue* queue) : Handler(queue) {
        player_ = player;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        switch (what) {
            case kStartPlayer:
                player_->StartPlayer();
                break;

            default:
                break;
        }
    }
private:
    VideoPlayer* player_;
};

}

#endif //TRINITY_VIDEO_PLAYER_H
