//
// Created by wlanjie on 2019-06-29.
//

#ifndef TRINITY_VIDEO_PLAYER_H
#define TRINITY_VIDEO_PLAYER_H

#include <android/native_window.h>
#include "audio_render.h"
#include "handler.h"
#include "gl.h"
#include "yuv_render.h"
#include "opengl.h"

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

typedef enum {
    kCreateEGLContext,
    kCreateWindowSurface,
    kDestroyWindowSurface,
    kDestroyEGLContext,
    kRenderFrame
} VideoRenderMessage;

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
class VideoRenderHandler;

class VideoPlayer {

public:
    VideoPlayer();
    virtual ~VideoPlayer();
    int Init();
    int Start(const char* file_name);
    void Stop();
    void Destroy();

    void OnSurfaceCreated(ANativeWindow* window);
    void OnSurfaceChanged(int width, int height);
    void OnSurfaceDestroyed();
public:
    int ReadAudio(uint8_t* buffer, int buffer_size);
    void StartPlayer();

private:
    static void* SyncThread(void* arg);
    void Sync();
    static void OnSeekEvent(SeekEvent* event, int seek_flag);
    static void OnAudioPrepareEvent(AudioEvent* event, int size);
    void StreamTogglePause(MediaDecode* media_decode, PlayerState* player_state);

    static void RenderVideoFrame(VideoEvent* event);
    static void *RenderThread(void *context);
    void ProcessMessage();
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

    VideoRenderHandler* handler_;
    MessageQueue* message_queue_;

    pthread_t render_thread_;
    pthread_mutex_t render_mutex_;
    pthread_cond_t render_cond_;

    int video_play_state_;
    EGLCore* core_;
    EGLSurface render_surface_;
    OpenGL* render_screen_;
    int surface_width_;
    int surface_height_;
    int frame_width_;
    int frame_height_;
    YuvRender* yuv_render_;
    ANativeWindow* window_;
    bool egl_context_exists_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    bool destroy_;
public:
    bool egl_destroy_;

    bool CreateEGLContext(ANativeWindow* window);

    void CreateWindowSurface(ANativeWindow* window);

    void DestroyWindowSurface();

    void DestroyEGLContext();

    void RenderVideo();
};


class VideoRenderHandler : public Handler {
public:
    VideoRenderHandler(VideoPlayer* player, MessageQueue* queue) : Handler(queue) {
        player_ = player;
        init_ = false;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        ANativeWindow* window;
        switch (what) {
            case kCreateEGLContext:
                if (player_->egl_destroy_) {
                    break;
                }
                window = (ANativeWindow *) (msg->GetObj());
                init_ = player_->CreateEGLContext(window);
                break;

            case kRenderFrame:
                if (player_->egl_destroy_) {
                    break;
                }
                if (init_) {
                    player_->RenderVideo();
                }
                break;

            case kCreateWindowSurface:
                if (player_->egl_destroy_) {
                    break;
                }
                if (init_) {
                    window = (ANativeWindow *) (msg->GetObj());
                    player_->CreateWindowSurface(window);
                }
                break;

            case kDestroyWindowSurface:
                if (init_) {
                    player_->DestroyWindowSurface();
                }
                break;

            case kDestroyEGLContext:
                player_->DestroyEGLContext();
                break;

            default:
                break;
        }
    }

private:
    VideoPlayer* player_;
    bool init_;
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
