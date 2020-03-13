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
#include "matrix.h"

namespace trinity {

enum VideoRenderMessage {
    kEGLCreate = 0,
    kRenderVideoFrame,
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
    , av_play_context_()
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
    , image_process_(nullptr)
    , music_player_(nullptr)
    , mutex_()
    , cond_()
    , window_created_(false)
    , file_name_(nullptr)
    , start_time_(0)
    , video_count_duration_(0)
    , end_time_(INT_MAX)
    , audio_render_(nullptr)
    , audio_buffer_size_(0)
    , audio_buffer_(nullptr)
    , draw_texture_id_(0) {
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
    av_play_context_->play_audio = PlayAudio;
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
    PostMessage(new Message(kEGLCreate));
}

Player::~Player() {
    LOGI("enter %s", __func__);
    PostMessage(new Message(kEGLDestroy));
    PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
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
    LOGI("leave %s", __func__);
}

void Player::ReleasePlayContext() {
    if (nullptr != av_play_context_) {
        av_play_release(av_play_context_);
        av_play_context_ = nullptr;
    }
}

int Player::Init() {
    return 0;
}

void Player::OnSurfaceCreated(ANativeWindow* window) {
    window_ = window;
    PostMessage(new Message(kEGLWindowCreate));
}

void Player::OnSurfaceChanged(int width, int height) {
    surface_width_ = width;
    surface_height_ = height;

    if (nullptr != render_screen_) {
        render_screen_->SetOutput(surface_width_, surface_height_);
    }
    if (frame_width_ != 0 && frame_height_ != 0) {
        SetFrame(frame_width_, frame_height_, surface_width_, surface_height_);
    }
    PostMessage(new Message(kSurfaceChanged));
}

void Player::OnSurfaceDestroy() {
    PostMessage(new Message(kEGLWindowDestroy));
}

void Player::OnComplete(AVPlayContext *context) {
    LOGI("enter %s", __func__);
    auto* player = reinterpret_cast<Player *>(context->priv_data);
    player->PostMessage(new Message(kPlayerStop));
    if (nullptr != player->player_event_observer_) {
        player->player_event_observer_->OnComplete();
    }
    LOGI("leave %s", __func__);
}

int Player::Start(const char *file_name, int start_time, int end_time, int video_count_duration) {
    LOGE("enter %s file_name: %s start_time: %d end_time: %d", __func__, file_name, start_time, end_time);
    delete[] file_name_;
    size_t len = strlen(file_name) + 1;
    file_name_ = new char[len];
    memset(file_name_, 0, len);
    memcpy(file_name_, file_name, len);
    start_time_ = start_time;
    end_time_ = end_time;
    video_count_duration_ = video_count_duration;
    if (window_created_) {
        PostMessage(new Message(kPlayerStart, start_time, end_time, (void *) file_name));
    }
    LOGI("leave %s", __func__);
    return 0;
}

void Player::Seek(int start_time, int end_time) {
    if (nullptr != av_play_context_) {
        av_play_seek(av_play_context_, start_time);
    }
}

void Player::Resume() {
    LOGI("Player Resume");
    PostMessage(new Message(kPlayerResume));
    if (nullptr != music_player_) {
        music_player_->Resume();
    }
}

void Player::Pause() {
    LOGI("Player Pause");
    PostMessage(new Message(kPlayerPause));
    if (nullptr != music_player_) {
        music_player_->Pause();
    }
}

void Player::Stop() {
    LOGI("enter %s", __func__);
    start_time_ = 0;
    end_time_ = INT_MAX;
    if (nullptr != file_name_) {
        delete[] file_name_;
        file_name_ = nullptr;
    }
    PostMessage(new Message(kPlayerStop));
    LOGI("leave %s", __func__);
}

void Player::Destroy() {
    LOGI("enter %s", __func__);
    Stop();
    PostMessage(new Message(kPlayerRelease));
    FreeMusicPlayer();
    LOGI("leave %s", __func__);
}

int64_t Player::GetCurrentPosition() {
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
    auto *message = new Message(kMusic, actId, 0, config);
    PostMessage(message);
    return actId;
}

void Player::UpdateMusic(const char* music_config, int action_id) {
    char *config = new char[strlen(music_config) + 1];
    sprintf(config, "%s%c", music_config, 0);
    auto *message = new Message(kMusicUpdate, action_id, 0, config);
    PostMessage(message);
}

void Player::DeleteMusic(int action_id) {
    auto *message = new Message(kMusicDelete, action_id, 0);
    PostMessage(message);
}

int Player::AddFilter(const char* filter_config) {
    int actId = current_action_id_++;
    char* config = new char[strlen(filter_config) + 1];
    sprintf(config, "%s%c", filter_config, 0);
    auto* message = new Message(kFilter, actId, 0, config);
    PostMessage(message);
    return actId;
}

void Player::UpdateFilter(const char* filter_config, int start_time, int end_time, int action_id) {
    char* config = new char[strlen(filter_config) + 1];
    sprintf(config, "%s%c", filter_config, 0);
    auto* message = new Message(kFilterUpdate, action_id, start_time, end_time, config);
    PostMessage(message);
}

void Player::DeleteFilter(int action_id) {
    auto* message = new Message(kFilterDelete, action_id, 0);
    PostMessage(message);
}

int Player::AddAction(const char *effect_config) {
    int actId = current_action_id_++;
    char *config = new char[strlen(effect_config) + 1];
    sprintf(config, "%s%c", effect_config, 0);
    auto *message = new Message(kEffect, actId, 0, config);
    PostMessage(message);
    return actId;
}

void Player::UpdateAction(int start_time, int end_time, int action_id) {
    auto *message = new Message(kEffectUpdate, start_time, end_time, action_id);
    PostMessage(message);
}

void Player::DeleteAction(int action_id) {
    auto *message = new Message(kEffectDelete, action_id, 0);
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
        if (nullptr != path_json) {
            music_player_->Start(path_json->valuestring);
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

void Player::PlayAudio(AVPlayContext* context) {
    auto* player = reinterpret_cast<Player*>(context->priv_data);
    player->PostMessage(new Message(kPlayAudio));
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
    context->audio_frame = frame_queue_get(context->audio_frame_queue);
    if (nullptr == context->audio_frame) {
        // 如果没有视频流  就从这里发结束信号
        if (context->eof && (((context->av_track_flags & VIDEO_FLAG) == 0) || context->just_audio)) {
            context->send_message(context, message_stop);
        }
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
                    rendering = false;
                }
                delete msg;
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
            char *file_name = reinterpret_cast<char *>(obj);
            LOGI("enter Start play: %d file: %s", av_play_context_->is_sw_decode, file_name);
            av_play_play(file_name, msg->GetArg1(), av_play_context_);
            if (nullptr != audio_render_) {
                delete audio_render_;
            }

            if (av_play_context_->av_track_flags & AUDIO_FLAG) {
                int channels = av_play_context_->audio_codec_context->channels <= 2
                        ? av_play_context_->audio_codec_context->channels : 2;
                audio_render_ = new AudioRender();
                audio_render_->Init(channels, av_play_context_->sample_rate, AudioCallback, this);
                audio_render_->Start();
            }
            PostMessage(new Message(kRenderVideoFrame));
            LOGI("leave Start play");
            break;
        }

        case kPlayAudio:
            if (nullptr != audio_render_) {
                audio_render_->Play();
            }
            break;

        case kPlayerResume:
            LOGE("enter kPlayerResume");
            if (av_play_context_ != nullptr && av_play_context_->status == PAUSED) {
                av_play_resume(av_play_context_);
            }
            if (audio_render_ != nullptr) {
                audio_render_->Play();
            }
            LOGE("leave kPlayerResume");
            break;

        case kPlayerPause:
            LOGI("enter kPlayerPause");
            if (nullptr != av_play_context_ && av_play_context_->status == PLAYING) {
                av_play_context_->change_status(av_play_context_, PAUSED);
            }
            if (audio_render_ != nullptr) {
                audio_render_->Pause();
            }
            LOGI("leave kPlayerPause");
            break;

        case kPlayerStop:
            LOGI("enter kPlayerStop");
            if (nullptr != av_play_context_) {
                av_play_stop(av_play_context_);
            }
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
            LOGI("leave kPlayerStop");
            break;

        case kPlayerRelease:
            if (nullptr != av_play_context_) {
                ReleasePlayContext();
            }
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
    if (av_play_context_->video_frame == nullptr) {
        av_play_context_->video_frame = frame_queue_get(av_play_context_->video_frame_queue);
    }
    if (!av_play_context_->video_frame) {
        if (av_play_context_->eof && av_play_context_->video_packet_queue->count == 0) {
            LOGE("video frame is NULL return -2");
            return -2;
        } else {
            LOGE("video frame is NULL return 0");
            usleep(10000);
            return 0;
        }
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
    if (diff >= WAIT_FRAME_SLEEP_US) {
        return -1;
    } else {
        if (!av_play_context_->is_sw_decode) {
            mediacodec_update_image(av_play_context_);
            int ret = mediacodec_get_texture_matrix(av_play_context_, texture_matrix_);
            if (ret != 0) {
                matrixSetIdentityM(texture_matrix_);
            }
        }
        int width = MIN(av_play_context_->video_frame->linesize[0], av_play_context_->video_frame->width);
        int height = av_play_context_->video_frame->height;
        if (frame_width_ != width || frame_height_ != height) {
            frame_width_ = width;
            frame_height_ = height;
            if (av_play_context_->is_sw_decode) {
                if (nullptr != yuv_render_) {
                    delete yuv_render_;
                }
                yuv_render_ = new YuvRender(0);
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
            usleep(diff);
        }
        if (av_play_context_->is_sw_decode) {
            draw_texture_id_ = yuv_render_->DrawFrame(av_play_context_->video_frame);
        } else {
            media_codec_render_->ActiveProgram();
            draw_texture_id_ = media_codec_render_->OnDrawFrame(oes_texture_, texture_matrix_);
        }
        ReleaseVideoFrame();

        if (draw_texture_id_ == -1) {
            return -1;
        }

        if (nullptr != image_process_) {
            int progressTextureId = image_process_->Process(draw_texture_id_,
                                                            static_cast<uint64_t>(GetCurrentPosition() - start_time_ + video_count_duration_),
                                                            frame_width_, frame_height_, 0, 0);
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
    if (EGL_NO_SURFACE == render_surface_) {
        LOGE("EGL_NO_SURFACE == render_surface_");
        return;
    }
    int texture = OnDrawFrame(texture_id, frame_width_, frame_height_);
    render_screen_->ActiveProgram();
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
    auto result = core_->InitWithSharedContext();
    if (!result) {
        LOGE("create EGLContext failed");
        return;
    }
    LOGI("leave %s", __func__);
}

void Player::OnGLWindowCreate() {
    LOGI("enter %s", __func__);
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
    core_->DoneCurrent();
    LOGI("render_surface: %p status: %d", render_surface_, av_play_context_->status);
    window_created_ = true;
    if (nullptr != file_name_ && av_play_context_->status == IDEL) {
        LOGE("nullptr != file_name_ && av_play_context_->status == IDEL");
        PostMessage(new Message(kPlayerStart, start_time_, end_time_, file_name_));
    }
    if (av_play_context_->status == PAUSED) {
        PostMessage(new Message(kRenderVideoFrame));
    }
    pthread_cond_signal(&cond_);
    LOGI("leave %s", __func__);
}

void Player::OnRenderVideoFrame() {
//    vm_->AttachCurrentThread(vm_, );
    if (nullptr == core_) {
        pthread_cond_wait(&cond_, &mutex_);
    }

    if (av_play_context_->status == PAUSED) {
        // TODO wait
        usleep(100000);
        PostMessage(new Message(kRenderVideoFrame));
    } else if (av_play_context_->status == PLAYING) {
        int ret = DrawVideoFrame();
        if (ret == 0) {
            PostMessage(new Message(kRenderVideoFrame));
        } else if (ret == -1) {
            usleep(WAIT_FRAME_SLEEP_US);
            PostMessage(new Message(kRenderVideoFrame));
            return;
        } else if (ret == -2) {
            av_play_context_->send_message(av_play_context_, message_stop);
            return;
        }
    } else if (av_play_context_->status == IDEL) {
        usleep(WAIT_FRAME_SLEEP_US);
    } else if (av_play_context_->status == BUFFER_EMPTY) {
        PostMessage(new Message(kRenderVideoFrame));
    }
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
