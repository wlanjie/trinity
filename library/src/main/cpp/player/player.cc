//
//  player.cc
//  player
//
//  Created by wlanjie on 2019/11/27.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <unistd.h>
#include "player.h"
#include "tools.h"

#define MAX_IMAGE_WIDTH 1080
#define MAX_IMAGE_HEIGHT 1920

namespace trinity {

enum VideoRenderMessage {
    kEGLCreate = 0,
    kRenderVideoFrame,
    kRenderImageFrame,
    kEGLDestroy,
    kEGLWindowCreate,
    kEGLWindowDestroy,
    kSurfaceChanged,
    kPlayerStart,
    kPlayerResume,
    kPlayerPause,
    kPlayerStop,
    kPlayerRelease,
    kPlayAudio
};

Player::Player(JNIEnv* env, jobject object) : Handler()
    , message_queue_thread_()
    , message_queue_()
    , av_play_context_(nullptr)
    , vm_()
    , object_()
    , core_(nullptr)
    , window_(nullptr)
    , render_surface_(EGL_NO_SURFACE)
    , yuv_render_(nullptr)
    , frame_width_(0)
    , frame_height_(0)
    , surface_width_(0)
    , surface_height_(0)
    , render_screen_(nullptr)
    , media_codec_render_(nullptr)
    , vertex_coordinate_(nullptr)
    , texture_coordinate_(nullptr)
    , player_event_observer_(nullptr)
    , current_action_id_(0)
    , texture_matrix_()
    , oes_texture_(0)
    , image_texture_(0)
    , image_frame_buffer_(nullptr)
    , image_render_start_time_(0)
    , image_process_(nullptr)
    , music_player_(nullptr)
    , mutex_()
    , cond_()
    , window_created_(false)
    , current_clip_(nullptr)
    , video_count_duration_(0)
    , audio_render_(nullptr)
    , audio_render_start_(false)
    , audio_buffer_size_(0)
    , audio_buffer_(nullptr)
    , draw_texture_id_(0)
    , player_state_(kNone)
    , current_time_(0)
    , previous_time_(0) {
    buffer_pool_ = new BufferPool(sizeof(Message));
    texture_matrix_ = new float[16];
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
    message_queue_ = new MessageQueue("Player Message Queue");
    InitMessageQueue(message_queue_);
    env->GetJavaVM(&vm_);
    object_ = env->NewGlobalRef(object);
    av_play_context_ = av_play_create(env, object_, 0, 44100);
    av_play_context_->priv_data = this;
    av_play_context_->on_complete = OnComplete;
    av_play_context_->change_status = OnStatusChanged;
//    av_play_context_->force_sw_decode = 1;
    av_play_set_buffer_time(av_play_context_, 5);

    vertex_coordinate_ = new GLfloat[8];
    vertex_coordinate_[0] = -1.0F;
    vertex_coordinate_[1] = -1.0F;
    vertex_coordinate_[2] = 1.0F;
    vertex_coordinate_[3] = -1.0F;
    vertex_coordinate_[4] = -1.0F;
    vertex_coordinate_[5] = 1.0F;
    vertex_coordinate_[6] = 1.0F;
    vertex_coordinate_[7] = 1.0F;

    texture_coordinate_ = new GLfloat[8];
    texture_coordinate_[0] = 0.0F;
    texture_coordinate_[1] = 0.0F;
    texture_coordinate_[2] = 1.0F;
    texture_coordinate_[3] = 0.0F;
    texture_coordinate_[4] = 0.0F;
    texture_coordinate_[5] = 1.0F;
    texture_coordinate_[6] = 1.0F;
    texture_coordinate_[7] = 1.0F;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&message_queue_thread_, &attr, MessageQueueThread, this);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEGLCreate;
    PostMessage(message);
}

Player::~Player() {
    LOGI("enter %s", __func__);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEGLDestroy;
    PostMessage(message);
    auto msg = buffer_pool_->GetBuffer<Message>();
    msg->what = MESSAGE_QUEUE_LOOP_QUIT_FLAG;
    PostMessage(msg);
    pthread_join(message_queue_thread_, nullptr);
    Destroy();
    ReleasePlayContext();
    if (nullptr != message_queue_) {
        message_queue_->Abort();
        delete message_queue_;
        message_queue_ = nullptr;
    }

    if (nullptr != vertex_coordinate_) {
        delete[] vertex_coordinate_;
        vertex_coordinate_ = nullptr;
    }
    if (nullptr != texture_coordinate_) {
        delete[] texture_coordinate_;
        texture_coordinate_ = nullptr;
    }
    if (nullptr != texture_matrix_) {
        delete[] texture_matrix_;
        texture_matrix_ = nullptr;
    }
    JNIEnv* env = nullptr;
    if ((vm_)->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        env->DeleteGlobalRef(object_);
    }
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
    if (nullptr != buffer_pool_) {
        delete buffer_pool_;
        buffer_pool_ = nullptr;
    }
    LOGI("leave %s", __func__);
}

void Player::ReleasePlayContext() {
    LOGI("enter: %s", __func__);
    if (nullptr != av_play_context_) {
        av_play_release(av_play_context_);
        av_play_context_ = nullptr;
    }
    LOGI("leave: %s", __func__);
}

int Player::Init() {
    return 0;
}

void Player::OnSurfaceCreated(ANativeWindow* window) {
    LOGI("enter: %s", __func__);
    window_ = window;
    if (nullptr != av_play_context_) {
        av_play_context_->window = window;
    }
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEGLWindowCreate;
    PostMessage(message);
    LOGI("leave: %s", __func__);
}

void Player::OnSurfaceChanged(int width, int height) {
    LOGI("enter: %s width: %d height: %d", __func__, width, height);
    surface_width_ = width;
    surface_height_ = height;

    if (nullptr != render_screen_) {
        render_screen_->SetOutput(surface_width_, surface_height_);
    }
    if (frame_width_ != 0 && frame_height_ != 0) {
        SetFrame(frame_width_, frame_height_, surface_width_, surface_height_);
    }
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kSurfaceChanged;
    PostMessage(message);
    LOGI("leave: %s", __func__);
}

void Player::OnSurfaceDestroy() {
    LOGI("enter: %s", __func__);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEGLWindowDestroy;
    PostMessage(message);
    LOGI("leave: %s", __func__);
}

void Player::OnComplete(AVPlayContext *context) {
    LOGI("enter %s", __func__);
    auto* player = reinterpret_cast<Player *>(context->priv_data);
    auto message = player->buffer_pool_->GetBuffer<Message>();
    message->what = kPlayerStop;
    player->PostMessage(message);
    if (nullptr != player->player_event_observer_) {
        player->player_event_observer_->OnComplete();
    }
    LOGI("leave %s", __func__);
}

void Player::OnStatusChanged(AVPlayContext *context, PlayStatus status) {
}

int Player::Start(MediaClip* clip, int video_count_duration) {
    LOGI("enter %s file_name: %s start_time: %lld end_time: %lld window_created: %d",
            __func__, clip->file_name, clip->start_time, clip->end_time, window_created_);
    player_state_ = kNone;
    current_clip_ = new MediaClip();
    current_clip_->start_time = clip->start_time;
    current_clip_->end_time = clip->end_time;
    current_clip_->type = clip->type;
    current_clip_->file_name = clip->file_name;

    video_count_duration_ = video_count_duration;
    if (window_created_) {
        auto message = buffer_pool_->GetBuffer<Message>();
        message->what = kPlayerStart;
        message->obj = current_clip_;
        PostMessage(message);
    }
    LOGI("leave %s", __func__);
    return 0;
}

void Player::Seek(int start_time, int end_time) {
    LOGI("enter: %s seek: %d end_time: %d", __func__, start_time, end_time);
    if (nullptr != av_play_context_) {
        av_play_seek(av_play_context_, start_time);
    }
    LOGI("leave: %s", __func__);
}

void Player::Resume() {
    LOGI("Player Resume");
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kPlayerResume;
    PostMessage(message);
    if (nullptr != music_player_) {
        music_player_->Resume();
    }
}

void Player::Pause() {
    LOGI("Player Pause");
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kPlayerPause;
    PostMessage(message);
    if (nullptr != music_player_) {
        music_player_->Pause();
    }
}

void Player::Stop() {
    LOGI("enter %s", __func__);
    if (nullptr != current_clip_) {
        delete current_clip_;
        current_clip_ = nullptr;
    }
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kPlayerStop;
    PostMessage(message);
    LOGI("leave %s", __func__);
}

void Player::Destroy() {
    LOGI("enter %s", __func__);
    Stop();
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kPlayerRelease;
    PostMessage(message);
    FreeMusicPlayer();
    LOGI("leave %s", __func__);
}

int64_t Player::GetCurrentPosition() {
    return current_time_;
}

int64_t Player::GetVideoDuration() {
    if (nullptr != av_play_context_) {
        if (av_play_context_->av_track_flags & AUDIO_FLAG) {
            return av_play_context_->audio_clock->pts / 1000;
        } else if (av_play_context_->av_track_flags & VIDEO_FLAG) {
            return av_play_context_->video_clock->pts / 1000;
        }
    }
    return 0;
}

void Player::AddObserver(trinity::PlayerEventObserver *observer) {
    player_event_observer_ = observer;
}

int Player::AddMusic(const char* music_config) {
    int actId = current_action_id_++;
    char *config = new char[strlen(music_config) + 1];
    sprintf(config, "%s%c", music_config, 0);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusic;
    message->arg1 = actId;
    message->obj = config;
    PostMessage(message);
    return actId;
}

void Player::UpdateMusic(const char* music_config, int action_id) {
    char *config = new char[strlen(music_config) + 1];
    sprintf(config, "%s%c", music_config, 0);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicUpdate;
    message->arg1 = action_id;
    message->obj = config;
    PostMessage(message);
    PostMessage(message);
}

void Player::DeleteMusic(int action_id) {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kMusicDelete;
    message->arg1 = action_id;
    PostMessage(message);
}

int Player::AddFilter(const char* filter_config) {
    int actId = current_action_id_++;
    char* config = new char[strlen(filter_config) + 1];
    sprintf(config, "%s%c", filter_config, 0);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kFilter;
    message->arg1 = actId;
    message->obj = config;
    PostMessage(message);
    return actId;
}

void Player::UpdateFilter(const char* filter_config, int start_time, int end_time, int action_id) {
    char* config = new char[strlen(filter_config) + 1];
    sprintf(config, "%s%c", filter_config, 0);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kFilterUpdate;
    message->arg1 = action_id;
    message->arg2 = start_time;
    message->arg3 = end_time;
    message->obj = config;
    PostMessage(message);
}

void Player::DeleteFilter(int action_id) {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kFilterDelete;
    message->arg1 = action_id;
    PostMessage(message);
}

int Player::AddAction(const char *effect_config) {
    int actId = current_action_id_++;
    char *config = new char[strlen(effect_config) + 1];
    sprintf(config, "%s%c", effect_config, 0);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEffect;
    message->arg1 = actId;
    message->obj = config;
    PostMessage(message);
    return actId;
}

void Player::UpdateAction(int start_time, int end_time, int action_id) {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEffectUpdate;
    message->arg1 = start_time;
    message->arg2 = end_time;
    message->arg3 = action_id;
    PostMessage(message);
}

void Player::DeleteAction(int action_id) {
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kEffectDelete;
    message->arg1 = action_id;
    PostMessage(message);
}

void Player::OnAddAction(char *config, int action_id) {
    if (nullptr != image_process_) {
        LOGI("add action id: %d config: %s", action_id, config);
        image_process_->OnAction(config, action_id);
    }
    delete[] config;
}

void Player::OnUpdateActionTime(int start_time, int end_time, int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("update action id: %d start_time: %d end_time: %d", action_id, start_time, end_time);
    image_process_->OnUpdateActionTime(start_time, end_time, action_id);
}

void Player::OnDeleteAction(int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("delete action id: %d", action_id);
    image_process_->RemoveAction(action_id);
}

void Player::OnAddMusic(char *config, int action_id) {
    FreeMusicPlayer();
    cJSON* music_config_json = cJSON_Parse(config);
    if (nullptr != music_config_json) {
        cJSON* path_json = cJSON_GetObjectItem(music_config_json, "path");
        cJSON* start_time_json = cJSON_GetObjectItem(music_config_json, "startTime");
        cJSON* end_time_json = cJSON_GetObjectItem(music_config_json, "endTime");

        music_player_ = new MusicDecoderController();
        music_player_->Init(0.2f, 44100);
        int start_time = 0;
        if (nullptr != start_time_json) {
            start_time = start_time_json->valueint;
        }
        // TODO time
        int end_time = INT32_MAX;
        if (nullptr != end_time_json) {
            end_time = end_time_json->valueint;
        }
        LOGI("music start_time: %d end_time: %d", start_time, end_time);
        if (nullptr != path_json) {
            char* path = path_json->valuestring;
            LOGI("music path: %s", path);
            music_player_->Start(path);
        }
    }

    delete[] config;
}

void Player::OnUpdateMusic(char *config, int action_id) {
    if (nullptr != music_player_) {
        music_player_->Stop();
        music_player_->Start("");
    }
}

void Player::OnDeleteMusic(int action_id) {
    FreeMusicPlayer();
}

void Player::FreeMusicPlayer() {
    if (nullptr != music_player_) {
        music_player_->Destroy();
        delete music_player_;
        music_player_ = nullptr;
    }
}

void Player::OnAddFilter(char* config, int action_id) {
    if (nullptr != image_process_) {
        LOGI("enter %s id: %d config: %s", __func__, action_id, config);
        image_process_->OnFilter(config, action_id);
        LOGI("leave %s", __func__);
    }
    delete[] config;
}

void Player::OnUpdateFilter(char* config, int action_id, int start_time, int end_time) {
    if (nullptr != image_process_) {
        LOGI("enter %s config: %s action_id: %d start_time: %d end_time: %d", __func__, config, action_id, start_time, end_time); // NOLINT
        image_process_->OnFilter(config, action_id, start_time, end_time);
        LOGI("leave %s", __func__);
    }
    delete[] config;
}

void Player::OnDeleteFilter(int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("enter %s action_id: %d", __func__, action_id);
    image_process_->OnDeleteFilter(action_id);
    LOGI("leave %s", __func__);
}

int Player::GetAudioFrame() {
    AVPlayContext* context = av_play_context_;
    if (nullptr == context) {
        return -1;
    }
    if (context->status == IDEL || context->status == PAUSED) {
        LOGE("GetAudioFrame error: %d", context->status);
        return -1;
    }
    if (context->audio_frame_queue->size == 0) {
        return 0;
    }
    context->audio_frame = frame_queue_get(context->audio_frame_queue);
    if (nullptr == context->audio_frame) {
        // 如果没有视频流  就从这里发结束信号
//        if (context->eof && (((context->av_track_flags & VIDEO_FLAG) == 0) || context->just_audio)) {
//            context->send_message(context, message_stop);
//        }
        LOGE("context->audio_frame is null");
        return -1;
    }
    // seek
    // get next frame
    while (context->audio_frame == &context->audio_frame_queue->flush_frame) {
        // 如果没有视频流  就从这里重置seek标记
        if ((context->av_track_flags & VIDEO_FLAG) == 0) {
            context->seeking = 0;
        }
        return GetAudioFrame();
    }

    int frame_size = av_samples_get_buffer_size(nullptr, context->audio_frame->channels,
            context->audio_frame->nb_samples, AV_SAMPLE_FMT_S16, 1);

    // filter will rewrite the frame's pts.  use  ptk_dts instead.
    int64_t time_stamp = av_rescale_q(context->audio_frame->pkt_dts,
                                      context->format_context->streams[context->audio_index]->time_base,
                                      AV_TIME_BASE_Q);
    if (audio_buffer_size_ < frame_size) {
        audio_buffer_size_ = frame_size;
        if (audio_buffer_ == nullptr) {
            audio_buffer_ = reinterpret_cast<uint8_t *>(malloc((size_t) audio_buffer_size_));
        } else {
            audio_buffer_ = reinterpret_cast<uint8_t *>(realloc(audio_buffer_,
                                                           (size_t) audio_buffer_size_));
        }
    }
    // TODO crash audio_buffer_ is nullptr
    if (frame_size > 0 && audio_buffer_ != nullptr) {
        memcpy(audio_buffer_, context->audio_frame->data[0], (size_t) frame_size);
    }
    frame_pool_unref_frame(context->audio_frame_pool, context->audio_frame);
    clock_set(context->audio_clock, time_stamp);
    return 0;
}

int Player::AudioCallback(uint8_t** buffer, int* buffer_size, void* context) {
    auto* player = reinterpret_cast<Player*>(context);
    int ret = player->GetAudioFrame();
    *buffer = player->audio_buffer_;
    *buffer_size = player->audio_buffer_size_;
    return ret;
}

void Player::ProcessMessage() {
    LOGI("enter %s", __func__);
    bool rendering = true;
    while (rendering) {
        Message* msg = nullptr;
        if (message_queue_->DequeueMessage(&msg, true) > 0) {
            if (nullptr != msg) {
                if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                    LOGE("MESSAGE_QUEUE_LOOP_QUIT_FLAG");
                    rendering = false;
                }
                if (nullptr != buffer_pool_) {
                    buffer_pool_->ReleaseBuffer(msg);
                }
            }
        }
    }
    LOGI("leave %s", __func__);
}

void* Player::MessageQueueThread(void *args) {
    auto* player = reinterpret_cast<Player*>(args);
    player->ProcessMessage();
    pthread_exit(nullptr);
}

void Player::HandleMessage(Message *msg) {
    int what = msg->GetWhat();
    void* obj = msg->GetObj();
    switch (what) {
        case kEGLCreate:
            OnGLCreate();
            break;

        case kEGLWindowCreate:
            OnGLWindowCreate();
            break;

        case kRenderVideoFrame:
            OnRenderVideoFrame();
            break;

        case kRenderImageFrame:
            OnRenderImageFrame();
            break;

        case kEGLWindowDestroy:
            OnGLWindowDestroy();
            break;

        case kSurfaceChanged:
            Draw(draw_texture_id_);
            break;

        case kEGLDestroy:
            OnGLDestroy();
            break;

        case kPlayerStart: {
            MediaClip* clip = reinterpret_cast<MediaClip*>(obj);
            LOGI("enter Start play: %d file: %s type: %d", av_play_context_->is_sw_decode, clip->file_name, clip->type);
            if (clip->type == VIDEO) {
                av_play_play(clip->file_name, msg->GetArg1(), av_play_context_);
                if (nullptr != audio_render_) {
                    delete audio_render_;
                }

                if (av_play_context_->av_track_flags & AUDIO_FLAG) {
                    audio_render_start_ = false;
                    int channels = av_play_context_->audio_codec_context->channels <= 2
                                   ? av_play_context_->audio_codec_context->channels : 2;
                    audio_render_ = new AudioRender();
                    audio_render_->Init(channels, av_play_context_->sample_rate, AudioCallback, this);
                }
                auto message = buffer_pool_->GetBuffer<Message>();
                message->what = kRenderVideoFrame;
                PostMessage(message);
            } else if (clip->type == IMAGE) {
                auto message = buffer_pool_->GetBuffer<Message>();
                message->what = kRenderImageFrame;
                PostMessage(message);
            }
            player_state_ = kStart;
            LOGI("leave Start play");
            break;
        }

        case kPlayAudio:
            if (nullptr != audio_render_) {
                audio_render_->Play();
            }
            break;

        case kPlayerResume:
            if (nullptr == current_clip_) {
                return;
            }
            LOGI("enter kPlayerResume clip_type: %d window_create: %d", current_clip_->type, window_created_);
            if (current_clip_->type == VIDEO) {
                if (av_play_context_ != nullptr && av_play_context_->status == PAUSED) {
                    av_play_resume(av_play_context_);
                }
                if (audio_render_ != nullptr) {
                    audio_render_->Play();
                }
                auto message = buffer_pool_->GetBuffer<Message>();
                message->what = kRenderVideoFrame;
                PostMessage(message);
            } else if (current_clip_->type == IMAGE) {
                auto message = buffer_pool_->GetBuffer<Message>();
                message->what = kRenderImageFrame;
                PostMessage(message);
            }
            player_state_ = window_created_ ? kResume : kNone;
            LOGI("leave kPlayerResume");
            break;

        case kPlayerPause:
            LOGI("enter kPlayerPause");
            if (current_clip_->type == VIDEO) {
                if (nullptr != av_play_context_ && av_play_context_->status == PLAYING) {
                    av_play_pause(av_play_context_);
                }
                if (audio_render_ != nullptr) {
                    audio_render_->Pause();
                }
            } else if (current_clip_->type == IMAGE) {

            }
            player_state_ = kPause;
            LOGI("leave kPlayerPause");
            break;

        case kPlayerStop:
            LOGI("enter kPlayerStop");
            if (nullptr != av_play_context_) {
                av_play_stop(av_play_context_);
            }
            audio_render_start_ = false;
            if (nullptr != audio_render_) {
                audio_render_->Stop();
                delete audio_render_;
                audio_render_ = nullptr;
            }
            if (nullptr != audio_buffer_) {
                free(audio_buffer_);
                audio_buffer_ = nullptr;
                audio_buffer_size_ = 0;
            }
            if (image_texture_ != 0) {
                glDeleteTextures(1, &image_texture_);
                image_texture_ = 0;
            }
            if (image_frame_buffer_ != nullptr) {
                delete image_frame_buffer_;
                image_frame_buffer_ = nullptr;
            }
            image_render_start_time_ = 0;
            frame_width_ = 0;
            frame_height_ = 0;
            previous_time_ = current_time_;
            if (previous_time_ >= video_count_duration_) {
                previous_time_ = 0;
            }
            player_state_ = kStop;
            LOGI("leave kPlayerStop");
            break;

        case kPlayerRelease:
            if (nullptr != av_play_context_) {
                ReleasePlayContext();
            }
            player_state_ = kNone;
            break;

        case kEffect:
            OnAddAction(static_cast<char *>(msg->GetObj()), msg->GetArg1());
            break;

        case kEffectUpdate:
            OnUpdateActionTime(msg->GetArg1(), msg->GetArg2(), msg->GetArg3());
            break;

        case kEffectDelete:
            OnDeleteAction(msg->GetArg1());
            break;

        case kMusic:
            OnAddMusic(static_cast<char *>(msg->GetObj()), msg->GetArg1());
            break;

        case kMusicUpdate:
            OnUpdateMusic(static_cast<char *>(msg->GetObj()), msg->GetArg1());
            break;

        case kMusicDelete:
            OnDeleteMusic(msg->GetArg1());
            break;

        case kFilter:
            OnAddFilter(static_cast<char *>(msg->GetObj()), msg->GetArg1());
            break;

        case kFilterUpdate:
            OnUpdateFilter(static_cast<char *>(msg->GetObj()), msg->GetArg1(), msg->GetArg2(), msg->GetArg3());
            break;

        case kFilterDelete:
            OnDeleteFilter(msg->GetArg1());
            break;
        default:
            break;
    }
}

void Player::SetFrame(int source_width, int source_height,
        int target_width, int target_height, RenderFrame frame_type) {
    float target_ratio = target_width * 1.0F / target_height;
    float scale_width = 1.0F;
    float scale_height = 1.0F;
    if (frame_type == FIT) {
        float source_ratio = source_width * 1.0F / source_height;
        if (source_ratio > target_ratio) {
            scale_width = 1.0F;
            scale_height = target_ratio / source_ratio;
        } else {
            scale_width = source_ratio / target_ratio;
            scale_height = 1.0F;
        }
    } else if (frame_type == CROP) {
        float source_ratio = source_width * 1.0F / source_height;
        if (source_ratio > target_ratio) {
            scale_width = source_ratio / target_ratio;
            scale_height = 1.0F;
        } else {
            scale_width = 1.0F;
            scale_height = target_ratio / source_ratio;
        }
    }
    vertex_coordinate_[0] = -scale_width;
    vertex_coordinate_[1] = -scale_height;
    vertex_coordinate_[2] = scale_width;
    vertex_coordinate_[3] = -scale_height;
    vertex_coordinate_[4] = -scale_width;
    vertex_coordinate_[5] = scale_height;
    vertex_coordinate_[6] = scale_width;
    vertex_coordinate_[7] = scale_height;
    LOGI("SetFrame source_width: %d source_height: %d target_width: %d target_height: %d scale_width; %f scale_height: %f",
            source_width, source_height, target_width, target_height, scale_width, scale_height); // NOLINT
}

int Player::OnDrawFrame(int texture_id, int width, int height) {
    JNIEnv* env = nullptr;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
        return 0;
    }
    if (nullptr == env) {
        LOGI("getJNIEnv failed");
        return 0;
    }
    int id = 0;
    jclass clazz = env->GetObjectClass(object_);
    if (nullptr != clazz) {
        jmethodID onDrawFrame = env->GetMethodID(clazz, "onDrawFrame", "(III)I");
        if (nullptr != onDrawFrame) {
            id = env->CallIntMethod(object_, onDrawFrame, texture_id, width, height);
        }
    }
    if (vm_->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
    return id;
}

int Player::DrawVideoFrame() {
    if (nullptr == av_play_context_) {
        LOGE("nullptr == av_play_context_");
        return -1;
    }
    if (!core_->MakeCurrent(render_surface_)) {
        LOGE("MakeCurrent error: %d", eglGetError());
        return -1;
    }
    if (av_play_context_->abort_request) {
        LOGE("player abort request");
        return -1;
    }
    if (av_play_context_->error_code == BUFFER_FLAG_END_OF_STREAM && av_play_context_->video_frame_queue->count == 0) {
        LOGE("av_play_context_->error_code == BUFFER_FLAG_END_OF_STREAM size: %d count: %d",
                av_play_context_->video_frame_queue->size, av_play_context_->video_frame_queue->count);
        return -2;
    }
    if (av_play_context_->video_frame == nullptr) {
        av_play_context_->video_frame = frame_queue_get(av_play_context_->video_frame_queue);
    }
    if (!av_play_context_->video_frame) {
        usleep(10000);
        return 0;
    }
    if (!audio_render_start_ && av_play_context_->av_track_flags & AUDIO_FLAG) {
        audio_render_start_ = true;
        audio_render_->Start();
    }
    // 发现seeking == 2 开始drop frame 直到发现&context->flush_frame
    // 解决连续seek卡住的问题
    if (av_play_context_->seeking == 2) {
        while (av_play_context_->video_frame != &av_play_context_->video_frame_queue->flush_frame) {
            ReleaseVideoFrame();
            av_play_context_->video_frame = frame_queue_get(av_play_context_->video_frame_queue);
            if (av_play_context_->video_frame == nullptr) {
                return 0;
            }
        }
        av_play_context_->seeking = 0;
        av_play_context_->video_frame = nullptr;
        clock_reset(av_play_context_->video_clock);
        return 0;
    }
    int64_t time_stamp;
    if (av_play_context_->is_sw_decode) {
        time_stamp = av_rescale_q(av_play_context_->video_frame->pts,
                av_play_context_->format_context->streams[av_play_context_->video_index]->time_base,
                AV_TIME_BASE_Q);
    } else {
        time_stamp = av_play_context_->video_frame->pts;
    }
    int64_t diff = 0;
    if (av_play_context_->av_track_flags & AUDIO_FLAG) {
        int64_t audio_delta_time = (audio_render_ == nullptr) ? 0 : audio_render_->GetDeltaTime();
        diff = time_stamp - (av_play_context_->audio_clock->pts + audio_delta_time);
    } else {
        diff = time_stamp - clock_get(av_play_context_->video_clock);
    }
    // diff >= 33ms if draw_mode == wait_frame return -1
    //              if draw_mode == fixed_frequency draw previous frame ,return 0
    // diff > 0 && diff < 33ms  sleep(diff) draw return 0
    // diff <= 0  draw return 0
    if (diff >= 1000000) {
        return -1;
    } else {
        int width = MIN(av_play_context_->video_frame->linesize[0], av_play_context_->video_frame->width);
        int height = av_play_context_->video_frame->height;
        if (frame_width_ != width || frame_height_ != height) {
            frame_width_ = width;
            frame_height_ = height;
            if (av_play_context_->is_sw_decode) {
                if (nullptr != yuv_render_) {
                    delete yuv_render_;
                }
                yuv_render_ = new YuvRender();
            } else {
                if (nullptr != media_codec_render_) {
                    delete media_codec_render_;
                }
                media_codec_render_ = new FrameBuffer(frame_width_, frame_height_,
                                  DEFAULT_VERTEX_MATRIX_SHADER, DEFAULT_OES_FRAGMENT_SHADER);
                media_codec_render_->SetTextureType(TEXTURE_OES);
            }
            if (surface_width_ != 0 && surface_height_ != 0) {
                SetFrame(frame_width_, frame_height_, surface_width_, surface_height_);
            }
        }
        if (diff > 0) {
            usleep(static_cast<useconds_t>(diff));
        }
        if (av_play_context_->is_sw_decode) {
            static GLfloat texture_coordinate[] = {
                    0.F, 1.F,
                    1.F, 1.F,
                    0.F, 0.F,
                    1.F, 0.F
            };
            // 有的视频会出现绿边, 使用矩阵放大, 去除绿边
            auto ratio = av_play_context_->video_frame->width * 1.F / av_play_context_->video_frame->linesize[0];
            glm::mat4 scale_matrix = glm::mat4(ratio);
            draw_texture_id_ = yuv_render_->DrawFrame(av_play_context_->video_frame, glm::value_ptr(scale_matrix),
                    DEFAULT_VERTEX_COORDINATE, texture_coordinate);
            current_time_ = av_rescale_q(av_play_context_->video_frame->pts,
                                         av_play_context_->format_context->streams[av_play_context_->video_index]->time_base,
                                         AV_TIME_BASE_Q) / 1000;
        } else {
            if (!mediacodec_frame_available(av_play_context_)) {
                LOGE("mediacodec frame is not available");
                draw_texture_id_ = -1;
            } else {
                mediacodec_update_image(av_play_context_);
                mediacodec_get_texture_matrix(av_play_context_, texture_matrix_);
//                for (int i = 0; i < 16; i += 4) {
//                    LOGE("m[%d]=%f [%d]=%f [%d]=%f [%d]=%f", i, texture_matrix_[i], i + 1,
//                         texture_matrix_[i + 1], i + 2, texture_matrix_[i + 2], i + 3,
//                         texture_matrix_[i + 3]);
//                }
                media_codec_render_->ActiveProgram();
                draw_texture_id_ = media_codec_render_->OnDrawFrame(oes_texture_, texture_matrix_);
                current_time_ = av_play_context_->video_frame->pts / 1000;
            }
        }
        ReleaseVideoFrame();

        if (draw_texture_id_ == -1) {
            return -1;
        }

        current_time_ = current_time_ + previous_time_;

        if (nullptr != image_process_) {
            int progressTextureId = image_process_->Process(draw_texture_id_,
                    current_time_, frame_width_, frame_height_, 0, 0);
            if (progressTextureId != 0) {
                draw_texture_id_ = progressTextureId;
            }
        }

        Draw(draw_texture_id_);
        clock_set(av_play_context_->video_clock, time_stamp);
    }
    return 0;
}

void Player::Draw(int texture_id) {
    if (nullptr == core_) {
        LOGE("nullptr == core_");
        return;
    }
    if (nullptr == render_screen_) {
        return;
    }
    if (EGL_NO_SURFACE == render_surface_) {
        LOGE("EGL_NO_SURFACE == render_surface_");
        return;
    }
    int texture = OnDrawFrame(texture_id, frame_width_, frame_height_);
    render_screen_->ActiveProgram();
    render_screen_->SetOutput(surface_width_, surface_height_);
    render_screen_->ProcessImage(static_cast<GLuint>(texture > 0 ? texture : texture_id),
            vertex_coordinate_, texture_coordinate_);
    if (!core_->SwapBuffers(render_surface_)) {
        LOGE("eglSwapBuffers error: %d", eglGetError());
    }
}

void Player::ReleaseVideoFrame() {
    if (!av_play_context_->is_sw_decode) {
        mediacodec_release_buffer(av_play_context_, av_play_context_->video_frame);
    }
    frame_pool_unref_frame(av_play_context_->video_frame_pool, av_play_context_->video_frame);
    av_play_context_->video_frame = nullptr;
}

void Player::OnGLCreate() {
    LOGI("enter %s", __func__);
    core_ = new EGLCore();
    auto result = core_->Init();
    if (!result) {
        LOGE("create EGLContext failed");
        return;
    }
    LOGI("leave %s", __func__);
}

void Player::OnGLWindowCreate() {
    LOGI("enter %s core_: %p", __func__, core_);
    render_surface_ = core_->CreateWindowSurface(window_);
    if (nullptr != render_surface_ && EGL_NO_SURFACE != render_surface_) {
        auto result = core_->MakeCurrent(render_surface_);
        if (!result) {
            LOGE("MakeCurrent error");
            return;
        }
    }
    if (nullptr == render_screen_) {
        render_screen_ = new OpenGL(surface_width_, surface_height_, DEFAULT_VERTEX_SHADER,
                                    DEFAULT_FRAGMENT_SHADER);
        render_screen_->SetOutput(surface_width_, surface_height_);
    }
    if (nullptr == image_process_) {
        image_process_ = new ImageProcess();
    }
    if (0 == oes_texture_) {
        glGenTextures(1, &oes_texture_);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, oes_texture_);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        av_play_context_->media_codec_texture_id = oes_texture_;
    }
    window_created_ = true;
    if (nullptr != current_clip_) {
        bool started_ = false;
        if (current_clip_->type == VIDEO) {
            started_ = av_play_context_->status == IDEL;
        } else if (current_clip_->type == IMAGE) {
            started_ = player_state_ == kNone;
        }
        if (started_) {
            auto message = buffer_pool_->GetBuffer<Message>();
            message->what = kPlayerStart;
            message->obj = current_clip_;
            PostMessage(message);
        }
    }
    if (av_play_context_->status == PAUSED) {
        auto message = buffer_pool_->GetBuffer<Message>();
        message->what = kRenderVideoFrame;
        PostMessage(message);
    }
    pthread_cond_signal(&cond_);
    LOGI("leave %s", __func__);
}

void Player::OnRenderVideoFrame() {
//    vm_->AttachCurrentThread(vm_, );
    if (nullptr == core_) {
        pthread_cond_wait(&cond_, &mutex_);
    }

    if (av_play_context_->status == PLAYING) {
        int ret = DrawVideoFrame();
        if (ret == 0) {
            auto message = buffer_pool_->GetBuffer<Message>();
            message->what = kRenderVideoFrame;
            PostMessage(message);
        } else if (ret == -1) {
            usleep(WAIT_FRAME_SLEEP_US);
            auto message = buffer_pool_->GetBuffer<Message>();
            message->what = kRenderVideoFrame;
            PostMessage(message);
            return;
        } else if (ret == -2) {
            av_play_context_->send_message(av_play_context_, message_stop);
            return;
        }
    } else if (av_play_context_->status == IDEL) {
        usleep(WAIT_FRAME_SLEEP_US);
    } else if (av_play_context_->status == BUFFER_EMPTY) {
        auto message = buffer_pool_->GetBuffer<Message>();
        message->what = kRenderVideoFrame;
        PostMessage(message);
    }
}

void Player::OnRenderImageFrame() {
    if (nullptr == current_clip_) {
        return;
    }
    if (player_state_ == kPause || player_state_ == kStop || player_state_ == kNone) {
        return;
    }
    if (!core_->MakeCurrent(render_surface_)) {
        LOGE("MakeCurrent error: %d", eglGetError());
        return;
    }
    if (image_texture_ == 0) {
        int width = 0;
        int height = 0;
        int channels = 0;
//        stbi_set_flip_vertically_on_load(1);
        auto image_data = stbi_load(current_clip_->file_name, &width, &height, &channels, STBI_rgb_alpha);
        if (width == 0 || height == 0 || nullptr == image_data) {
            return;
        }
        LOGI("Decode image width: %d height: %d channels: %d", width, height, channels);
        glGenTextures(1, &image_texture_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, image_texture_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        frame_width_ = width;
        frame_height_ = height;

//        if (width > MAX_IMAGE_WIDTH || height > MAX_IMAGE_HEIGHT) {
//            // 当图片大于1080p时, 缩放到1080p
//            auto resize_width_ratio = MAX_IMAGE_WIDTH * 1.0F / width;
//            auto resize_height_ratio = MAX_IMAGE_HEIGHT * 1.0F / height;
//            auto resize_width = static_cast<int>(MIN(resize_width_ratio, resize_height_ratio) * width);
//            auto resize_height = static_cast<int>(MIN(resize_width_ratio, resize_height_ratio) * height);
//            auto resize_image_data = reinterpret_cast<unsigned char*>(
//                    malloc(static_cast<size_t>(resize_width * resize_height * channels)));
//            stbir_resize_uint8(image_data, width, height, 0, resize_image_data, resize_width, resize_height, 0, channels);
//            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resize_width, resize_height,
//                    0, GL_RGBA, GL_UNSIGNED_BYTE, resize_image_data);
//            free(resize_image_data);
//            frame_width_ = resize_width;
//            frame_height_ = resize_height;
//        } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
//        }
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(image_data);
        image_frame_buffer_ = new FrameBuffer(frame_width_, frame_height_, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);

        if (surface_width_ > 0 && surface_height_ > 0) {
            SetFrame(frame_width_, frame_height_, surface_width_, surface_height_);
        }
    }

    if (image_render_start_time_ == 0) {
        image_render_start_time_ = getCurrentTime();
    }

    auto current_time = getCurrentTime() - image_render_start_time_;
    if (current_time >= current_clip_->end_time) {
        OnComplete(av_play_context_);
        return;
    }

    static GLfloat texture_coordinate[] = {
            0.F, 1.F,
            1.F, 1.F,
            0.F, 0.F,
            1.F, 0.F
    };
    current_time_ = current_time + previous_time_;
    // 图片时需要上下镜像一下
    draw_texture_id_ = image_frame_buffer_->OnDrawFrame(image_texture_, DEFAULT_VERTEX_COORDINATE, texture_coordinate);
    if (nullptr != image_process_) {
        int progressTextureId = image_process_->Process(draw_texture_id_,
                    current_time_, frame_width_, frame_height_, 0, 0);
        if (progressTextureId != 0) {
            draw_texture_id_ = progressTextureId;
        }
    }
    Draw(draw_texture_id_);
    // 图片按25帧刷新
    usleep(40 * 1000);
    auto message = buffer_pool_->GetBuffer<Message>();
    message->what = kRenderImageFrame;
    PostMessage(message);
}

void Player::OnGLWindowDestroy() {
    LOGI("enter %s", __func__);
    if (nullptr != core_ && EGL_NO_SURFACE != render_surface_) {
        core_->ReleaseSurface(render_surface_);
        render_surface_ = EGL_NO_SURFACE;
    }
    frame_width_ = 0;
    frame_height_ = 0;
    window_created_ = false;
    LOGI("leave %s", __func__);
}

void Player::OnGLDestroy() {
    LOGI("enter %s", __func__);
    if (nullptr != image_process_) {
        delete image_process_;
        image_process_ = nullptr;
    }
    if (nullptr != render_screen_) {
        delete render_screen_;
        render_screen_ = nullptr;
    }
    if (nullptr != media_codec_render_) {
        delete media_codec_render_;
        media_codec_render_ = nullptr;
    }
    if (nullptr != yuv_render_) {
        delete yuv_render_;
        yuv_render_ = nullptr;
    }
    if (0 != oes_texture_) {
        glDeleteTextures(1, &oes_texture_);
        oes_texture_ = 0;
    }
    if (nullptr != core_) {
        if (EGL_NO_SURFACE != render_surface_) {
            LOGE("%s MakeCurrent: %p", __func__, render_surface_);
            core_->MakeCurrent(render_surface_);
        }
        core_->Release();
        delete core_;
        core_ = nullptr;
    }
    LOGI("leave %s", __func__);
}

}  // namespace trinity
