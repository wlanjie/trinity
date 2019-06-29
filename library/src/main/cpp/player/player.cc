//
// Created by Edtion on 2019-06-22.
//

#include "player.h"
#include "android_xlog.h"

namespace trinity {

Player::Player() {
    repeat_ = false;
    destroy_ = false;
    video_state_ = nullptr;
    context_ = nullptr;

    player_queue_ = new MessageQueue("Player Message Queue");
    player_handler_ = new PlayerHandler(this, player_queue_);
    audio_render_ = new AudioRender();
}

Player::~Player() {
    if (nullptr != player_queue_) {
        delete player_queue_;
        player_queue_ = nullptr;
    }
    if (nullptr != player_handler_) {
        delete player_handler_;
        player_handler_ = nullptr;
    }
    if (nullptr != audio_render_) {
        delete audio_render_;
        audio_render_ = nullptr;
    }
}

int Player::Init(void* start, void* render, void* context) {

    context_ = new PlayerActionContext();
    context_->PlayerStartPlay = (void(*)(PlayerActionContext *context))start;
    context_->PlayerRenderVideo = (void(*)(PlayerActionContext *context))render;
    context_->context = context;

    int result = pthread_create(&player_thread_, nullptr, PlayerThread, this);
    if (result != 0) {
        LOGE("Init player thread error: %d", result);
        return false;
    }
    return result;
}

void* Player::PlayerThread(void *context) {
    Player* player = (Player*) context;
    if (player->destroy_) {
        return nullptr;
    }
    player->ProcessPlayerMessage();
    pthread_exit(0);
}

void Player::ProcessPlayerMessage() {
    bool rendering = true;
    while (rendering) {
        if (destroy_) {
            break;
        }
        Message *msg = NULL;
        if (player_queue_->DequeueMessage(&msg, true) > 0) {
            if (msg == NULL) {
                return;
            }
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                rendering = false;
            }
            delete msg;
        }
    }
}

int Player::Play(bool repeat, PlayerClip *clip) {
    repeat_ = repeat;
    InitPlayer(clip);
    return 0;
}

void Player::Pause() {
    stream_toggle_pause(video_state_);
}

void Player::Resume() {
    stream_toggle_pause(video_state_);
}

void Player::Destroy() {

    if (nullptr != video_state_) {
        if (nullptr != video_state_->audio_render_context) {
            av_free(video_state_->audio_render_context);
            video_state_->audio_render_context = nullptr;
        }
        if (nullptr != video_state_->video_render_context) {
            av_free(video_state_->video_render_context);
            video_state_->video_render_context = nullptr;
        }
        if (nullptr != video_state_->state_context) {
            av_free(video_state_->state_context);
            video_state_->state_context = nullptr;
        }
        av_destroy(video_state_);
        av_freep(video_state_);
        video_state_ = nullptr;
    }

    if (nullptr != context_) {
        free(context_);
        context_ = nullptr;
    }

    if (nullptr != audio_render_) {
        audio_render_->Stop();
    }
}

static int AudioCallback(uint8_t* buffer, size_t buffer_size, void* context) {
    Player* player = (Player*) context;
    return player->ReadAudio(buffer, buffer_size);
}

static int OnCompleteState(StateContext *context) {
    Player* player = (Player*) context->context;
    return player->OnComplete();
}

int Player::OnComplete() {
    if (nullptr == video_state_) {
        return 1;
    }
    player_handler_->PostMessage(new Message(kStartPlayer));
    // 是否退出播放
    return 0;
}

void Player::StartPlayer() {
    if (nullptr != context_ && nullptr != context_->PlayerStartPlay) {
        context_->PlayerStartPlay(context_);
    }
}


// Player method
int Player::InitPlayer(PlayerClip* clip) {
    destroy_ = false;

    video_state_ = (VideoState*) av_malloc(sizeof(VideoState));
    video_state_->video_render_context = (VideoRenderContext*) av_malloc(sizeof(VideoRenderContext));
    video_state_->video_render_context->render_video_frame = RenderVideoFrame;
    video_state_->video_render_context->context = this;

    StateContext* state_context = (StateContext*) av_malloc(sizeof(StateContext));
    state_context->complete = OnCompleteState;
    state_context->context = this;
    video_state_->state_context = state_context;

    video_state_->precision_seek = 1;
    video_state_->loop = 1;
    video_state_->start_time = clip->start_time;
    video_state_->end_time = clip->end_time == 0 ? INT64_MAX : clip->end_time;

    av_start(video_state_, clip->file_name);
    audio_render_->Init(1, 44100, AudioCallback, this);
    audio_render_->Start();
    return 0;
}

int Player::ReadAudio(uint8_t *buffer, int buffer_size) {
    if (nullptr == video_state_) {
        return 0;
    }
    VideoState* is = video_state_;
    int audio_size, len1 = 0;
    while (buffer_size > 0) {
        if (is->audio_buf_index >= is->audio_buf_size) {
            audio_size = audio_decoder_frame(is);
            if (audio_size < 0) {
                is->audio_buf = nullptr;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            } else {
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > buffer_size) {
            len1 = buffer_size;
        }
        if (nullptr == is->audio_buf) {
            memset(buffer, 0, len1);
            break;
        } else {
            memcpy(buffer, is->audio_buf + is->audio_buf_index, len1);
        }
        buffer_size -= len1;
        buffer += len1;
        is->audio_buf_index += len1;
    }
    return len1;
}

void Player::RenderVideoFrame(VideoRenderContext* context) {
    if (nullptr != context && nullptr != context->context) {
        Player* player = (Player*) context->context;

        if (nullptr != player->context_) {
            player->context_->PlayerRenderVideo(player->context_);
        }
    }
}

}