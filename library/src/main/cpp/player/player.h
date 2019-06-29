//
// Created by Edtion on 2019-06-22.
//

#ifndef TRINITY_PLAYER_H
#define TRINITY_PLAYER_H

extern "C" {
#include "ffplay.h"
};

#include <cstdint>
#include <jni.h>

#include "handler.h"
#include "audio_render.h"

using namespace std;

namespace trinity {

class PlayerHandler;

typedef struct {
    char* file_name;
    int64_t start_time;
    int64_t end_time;
} PlayerClip;

typedef enum {
    kNone = 0,
    kStartPlayer,
    // play state
    kPlaying,
    kResume,
    kPause,
    kStop
} PlayerMessage;

typedef struct PlayerActionContext {
    void (*PlayerStartPlay)(struct PlayerActionContext* context);
    void (*PlayerRenderVideo)(struct PlayerActionContext* context);
    void* context;
} PlayerActionContext;

class Player {

public:
    Player();
    ~Player();

    int Init(void* start, void* render, void* context);

    // 开始播放
    // 是否循环播放
    int Play(bool repeat, PlayerClip *clip);

    // 暂停播放
    void Pause();

    // 继续播放
    void Resume();

    // 停止播放
    void Stop();

    void Destroy();

private:

    static void* PlayerThread(void* context);

    void ProcessPlayerMessage();


public:
    // player
    int InitPlayer(PlayerClip *clip);

    int ReadAudio(uint8_t* buffer, int buffer_size);

    static void RenderVideoFrame(VideoRenderContext* context);

    int OnComplete();

    void StartPlayer();

public:
    VideoState* video_state_;
    AudioRender* audio_render_;

private:
    // 是否循环播放
    bool repeat_;
    bool destroy_;


    pthread_t player_thread_;
    PlayerHandler* player_handler_;
    MessageQueue* player_queue_;

    PlayerActionContext* context_;
};

class PlayerHandler : public Handler {
public:
    PlayerHandler(Player* player, MessageQueue* queue) : Handler(queue) {
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
    Player* player_;
};

}


#endif //TRINITY_PLAYER_H
