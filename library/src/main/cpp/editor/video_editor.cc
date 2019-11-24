/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by wlanjie on 2019-05-14.
//

#include "video_editor.h"
#include "android_xlog.h"
#include "gl.h"

namespace trinity {

VideoEditor::VideoEditor(const char* resource_path) : current_action_id_(0) {
    window_ = nullptr;
    video_editor_object_ = nullptr;
    repeat_ = false;
    play_index = 0;
    av_play_context_ = nullptr;
    editor_resource_ = new EditorResource(resource_path);
    music_player_ = nullptr;
//    state_event_ = nullptr;
//    on_video_render_event_ = nullptr;
//
//    message_queue_ = new MessageQueue("Video Complete Message Queue");
//    handler_ = new PlayerHandler(this, message_queue_);
}

VideoEditor::~VideoEditor() {
//    if (nullptr != video_player_) {
//        delete video_player_;
//        video_player_ = nullptr;
//    }
    if (nullptr != editor_resource_) {
        delete editor_resource_;
        editor_resource_ = nullptr;
    }
    if (nullptr != message_queue_) {
        message_queue_->Abort();
        delete message_queue_;
        message_queue_ = nullptr;
    }
}

int VideoEditor::Init() {
    int result = pthread_mutex_init(&queue_mutex_, nullptr);
    if (result != 0) {
        LOGE("init clip queue mutex error");
        return result;
    }
    result = pthread_cond_init(&queue_cond_, nullptr);
    if (result != 0) {
        LOGE("init clip queue cond error");
        return result;
    }
    return result;
}

void VideoEditor::OnSurfaceCreated(JNIEnv* env, jobject object, jobject surface) {
    if (nullptr != window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
    if (nullptr != video_editor_object_) {
        env->DeleteGlobalRef(video_editor_object_);
        video_editor_object_ = nullptr;
    }
    window_ = ANativeWindow_fromSurface(env, surface);
    video_editor_object_ = env->NewGlobalRef(object);
    av_play_context_->video_render_context->set_window(av_play_context_->video_render_context, window_);
}

void VideoEditor::OnSurfaceChanged(int width, int height) {
    if (nullptr != av_play_context_ && nullptr != av_play_context_->video_render_context) {
        if (av_play_context_->video_render_context->model != NULL) {
            Model *model = av_play_context_->video_render_context->model;
            model->resize(model, width, height);
        }
        av_play_context_->video_render_context->width = width;
        av_play_context_->video_render_context->height = height;
    }
}

void VideoEditor::OnSurfaceDestroyed(JNIEnv* env) {
    if (nullptr != video_editor_object_) {
        env->DeleteGlobalRef(video_editor_object_);
        video_editor_object_ = nullptr;
    }
}

int64_t VideoEditor::GetVideoDuration() const {
    int64_t duration = 0;
    for (int i = 0; i < clip_deque_.size(); ++i) {
        MediaClip* clip = clip_deque_.at(i);
        duration += clip->end_time - clip->start_time;
    }
    return duration;
}

int64_t VideoEditor::GetCurrentPosition() const {
    if (nullptr != av_play_context_) {
        if (av_play_context_->av_track_flags & AUDIO_FLAG) {
            return av_play_context_->audio_clock->pts / 1000000;
        } else if (av_play_context_->av_track_flags & VIDEO_FLAG) {
            return av_play_context_->video_clock->pts / 1000000;
        }
    }
    return 0;
}

int VideoEditor::GetClipsCount() {
    pthread_mutex_lock(&queue_mutex_);
    int size = clip_deque_.size();
    pthread_mutex_unlock(&queue_mutex_);
    return size;
}

MediaClip *VideoEditor::GetClip(int index) {
    if (index < 0 || index >= clip_deque_.size()) {
        return nullptr;
    }
    return clip_deque_.at(index);
}

int VideoEditor::InsertClip(MediaClip *clip) {
    pthread_mutex_lock(&queue_mutex_);
    clip_deque_.push_back(clip);
    editor_resource_->InsertClip(clip);
    pthread_mutex_unlock(&queue_mutex_);
    return 0;
}

int VideoEditor::InsertClipWithIndex(int index, MediaClip *clip) {
    return 0;
}

void VideoEditor::RemoveClip(int index) {
    if (index < 0 || index > clip_deque_.size()) {
        return;
    }
    editor_resource_->RemoveClip(index);
    MediaClip* clip = clip_deque_.at(index);
    delete clip;
    clip_deque_.erase(clip_deque_.begin() + index);
}

int VideoEditor::ReplaceClip(int index, MediaClip *clip) {
    editor_resource_->ReplaceClip(index, clip);
    return 0;
}

int64_t VideoEditor::GetVideoTime(int index, int64_t clip_time) {
    if (index < 0 || index >= clip_deque_.size()) {
        return 0;
    }
    int64_t offset = 0;
    for (int i = 0; i < index; ++i) {
        MediaClip* clip = clip_deque_.at(i);
        offset += (clip->end_time - clip->start_time);
    }
    return clip_time + offset;
}

int64_t VideoEditor::GetClipTime(int index, int64_t video_time) {
    if (index < 0 || index >= clip_deque_.size()) {
        return 0;
    }
    int64_t offset = 0;
    for (int i = 0; i < index; ++i) {
        MediaClip* clip = clip_deque_.at(i);
        offset += (clip->end_time - clip->start_time);
    }
    return video_time - offset;
}

int VideoEditor::GetClipIndex(int64_t time) {
    return 0;
}

int VideoEditor::AddFilter(const char* filter_config) {
    LOGI("add filter config: %s", filter_config);
//    if (nullptr != video_player_) {
//        int actId = current_action_id_++;
//        char* config = new char[strlen(filter_config) + 1];
//        // TODO snprintf
//        sprintf(config, "%s%c", filter_config, 0);
//        Message* message = new Message(kFilter, actId, 0, config);
//        video_player_->SendGLMessage(message);
//        return actId;
//    }
    return -1;
}

void VideoEditor::UpdateFilter(const char *filter_config, int action_id) {
    LOGI("update filter action_id: %d config: %s", action_id, filter_config);
//    if (nullptr != video_player_) {
//        char* config = new char[strlen(filter_config) + 1];
//        // TODO snprintf
//        sprintf(config, "%s%c", filter_config, 0);
//        Message* message = new Message(kFilter, action_id, 0, config);
//        video_player_->SendGLMessage(message);
//    }
}

void VideoEditor::DeleteFilter(int action_id) {
//    if (nullptr != video_player_) {
//        // TODO delete
//        Message* message = new Message();
//    }
}

int VideoEditor::AddMusic(const char* music_config) {
//    if (nullptr != video_player_) {
//        int actId = current_action_id_++;
//        char *config = new char[strlen(music_config) + 1];
//        sprintf(config, "%s%c", music_config, 0);
//        Message *message = new Message(kMusic, actId, 0, config);
//        video_player_->SendGLMessage(message);
//
//        if (nullptr != editor_resource_) {
//            editor_resource_->AddMusic(music_config, actId);
//        }
//        return actId;
//    }
    return -1;
}

void VideoEditor::UpdateMusic(const char* music_config, int action_id) {
//    if (nullptr != video_player_) {
//        char *config = new char[strlen(music_config) + 1];
//        sprintf(config, "%s%c", music_config, 0);
//        Message *message = new Message(kMusicUpdate, action_id, 0, config);
//        video_player_->SendGLMessage(message);
//    }
//    if (nullptr != editor_resource_) {
//        editor_resource_->UpdateMusic(music_config, action_id);
//    }
}

void VideoEditor::DeleteMusic(int action_id) {
//    if (nullptr != video_player_) {
//        Message *message = new Message(kMusicDelete, action_id, 0);
//        video_player_->SendGLMessage(message);
//    }
//    if (nullptr != editor_resource_) {
//        editor_resource_->DeleteMusic(action_id);
//    }
}

int VideoEditor::AddAction(const char *effect_config) {
//    if (nullptr != video_player_) {
//        int actId = current_action_id_++;
//        char *config = new char[strlen(effect_config) + 1];
//        sprintf(config, "%s%c", effect_config, 0);
//        Message *message = new Message(kEffect, actId, 0, config);
//        video_player_->SendGLMessage(message);
//
//        if (nullptr != editor_resource_) {
//            editor_resource_->AddAction(effect_config, actId);
//        }
//        return actId;
//    }
    return -1;
}

void VideoEditor::UpdateAction(const char *effect_config, int action_id) {
//    if (nullptr != video_player_) {
//        char *config = new char[strlen(effect_config) + 1];
//        sprintf(config, "%s%c", effect_config, 0);
//        Message *message = new Message(kEffectUpdate, action_id, 0, config);
//        video_player_->SendGLMessage(message);
//    }
//    if (nullptr != editor_resource_) {
//        editor_resource_->UpdateAction(effect_config, action_id);
//    }
}

void VideoEditor::DeleteAction(int action_id) {
//    if (nullptr != video_player_) {
//        Message *message = new Message(kEffectDelete, action_id, 0);
//        video_player_->SendGLMessage(message);
//    }
    if (nullptr != editor_resource_) {
        editor_resource_->DeleteAction(action_id);
    }
}

//int VideoEditor::OnCompleteEvent(StateEvent *event) {
//    VideoEditor *editor = reinterpret_cast<VideoEditor*>(event->context);
//    if (nullptr != editor) {
//        editor->handler_->PostMessage(new Message(kStartPlayer));
//    }
//    return 0;
//}

//int VideoEditor::OnVideoRender(OnVideoRenderEvent *event, int texture_id, int width, int height, uint64_t current_time) {
//    VideoEditor *editor = reinterpret_cast<VideoEditor*>(event->context);
//    if (nullptr != editor) {
//        return editor->image_process_->Process(texture_id, current_time, width, height, 0, 0);
//    }
//    return 0;
//}

int VideoEditor::OnComplete() {
//    if (repeat_) {
//        if (clip_deque_.size() == 1) {
//            MediaClip* clip = clip_deque_.at(0);
//            video_player_->Seek(clip->start_time);
//        } else {
//            video_player_->Stop();
//            play_index++;
//            if (play_index >= clip_deque_.size()) {
//                play_index = 0;
//            }
//            MediaClip* clip = clip_deque_.at(play_index);
//            FreeStateEvent();
//            AllocStateEvent();
//            video_player_->Start(clip->file_name, clip->start_time,
//                    clip->end_time == 0 ? INT64_MAX : clip->end_time, state_event_, on_video_render_event_);
//        }
//    } else {
//        video_player_->Stop();
//    }
    return 0;
}

void VideoEditor::FreeMusicPlayer() {
    if (nullptr != music_player_) {
        music_player_->Destroy();
        delete music_player_;
        music_player_ = nullptr;
    }
}

void VideoEditor::FreeStateEvent() {
//    if (nullptr != state_event_) {
//        av_free(state_event_);
//        state_event_ = nullptr;
//    }
}

void VideoEditor::AllocStateEvent() {
//    state_event_ = reinterpret_cast<StateEvent*>(av_malloc(sizeof(StateEvent)));
//    memset(state_event_, 0, sizeof(StateEvent));
//    state_event_->on_complete_event = OnCompleteEvent;
//    state_event_->context = this;
}

void VideoEditor::FreeVideoRenderEvent() {
//    if (nullptr != on_video_render_event_) {
//        av_free(on_video_render_event_);
//        on_video_render_event_ = nullptr;
//    }
}

void VideoEditor::AllocVideoRenderEvent() {
//    on_video_render_event_ = reinterpret_cast<OnVideoRenderEvent*>(av_malloc(sizeof(OnVideoRenderEvent)));
//    memset(on_video_render_event_, 0, sizeof(OnVideoRenderEvent));
//    on_video_render_event_->on_render_video = OnVideoRender;
//    on_video_render_event_->context = this;
}

void VideoEditor::Seek(int time) {
//    if (nullptr != video_player_) {
//        video_player_->Seek(time);
//    }
}

int VideoEditor::Play(bool repeat, JNIEnv* env, jobject object) {
    if (clip_deque_.empty()) {
        return -1;
    }
    FreeStateEvent();
    AllocStateEvent();
    FreeVideoRenderEvent();
    AllocVideoRenderEvent();
    repeat_ = repeat;
    MediaClip* clip = clip_deque_.at(0);
    if (nullptr == av_play_context_) {
        av_play_context_ = av_play_create(env, object, 0, 44100);
    }
    av_play_context_->is_sw_decode = true;
    av_play_context_->video_render_context->change_model(av_play_context_->video_render_context, Rect);
    av_play_play(clip->file_name, 0, av_play_context_);
    return 0;
}

void VideoEditor::Pause() {
    if (nullptr != av_play_context_ && av_play_context_->status == PLAYING) {
        av_play_context_->change_status(av_play_context_, PAUSED);
    }
    if (nullptr != music_player_) {
        music_player_->Pause();
    }
}

void VideoEditor::Resume() {
    if (nullptr != av_play_context_) {
        av_play_resume(av_play_context_);
    }
    if (nullptr != music_player_) {
        music_player_->Resume();
    }
}

void VideoEditor::Stop() {}

void VideoEditor::Destroy() {
    LOGI("enter Destroy");
    if (nullptr != av_play_context_) {
        av_play_stop(av_play_context_);
        av_play_release(av_play_context_);
    }

    pthread_mutex_lock(&queue_mutex_);
    for (int i = 0; i < clip_deque_.size(); ++i) {
        MediaClip* clip = clip_deque_.at(i);
        delete[] clip->file_name;
        delete clip;
    }
    clip_deque_.clear();
    pthread_mutex_unlock(&queue_mutex_);
    LOGE("start queue_mutex_ destroy");
    pthread_mutex_destroy(&queue_mutex_);
    pthread_cond_destroy(&queue_cond_);
    FreeStateEvent();
    FreeVideoRenderEvent();
//    handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(complete_thread_, nullptr);
    FreeMusicPlayer();
    LOGE("leave Destroy");
}

void* VideoEditor::CompleteThread(void* context) {
    VideoEditor* video_editor = static_cast<VideoEditor *>(context);
    video_editor->ProcessMessage();
    pthread_exit(0);
}

void VideoEditor::ProcessMessage() {
    bool running = true;
    while (running) {
        Message* msg = nullptr;
        if (message_queue_->DequeueMessage(&msg, true) > 0) {
            if (msg == nullptr) {
                return;
            }
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                running = false;
            }
            delete msg;
        }
    }
}

void VideoEditor::OnGLCreate() {
    image_process_ = new ImageProcess();
}

void VideoEditor::OnGLMessage(trinity::Message *msg) {
    switch (msg->GetWhat()) {
//        case kFilter:
//            if (nullptr != image_process_) {
//                char* config = static_cast<char *>(msg->GetObj());
//                image_process_->OnFilter(config, msg->GetArg1());
//                delete[] config;
//            }
//            break;
//        case kEffect:
//            OnAddAction(static_cast<char *>(msg->GetObj()), msg->GetArg1());
//            break;
//
//        case kEffectUpdate:
//            OnUpdateAction(static_cast<char *>(msg->GetObj()), msg->GetArg1());
//            break;
//
//        case kEffectDelete:
//            OnDeleteAction(msg->GetArg1());
//            break;
//
//        case kMusic:
//            OnAddMusic(static_cast<char *>(msg->GetObj()), msg->GetArg1());
//            break;
//
//        case kMusicUpdate:
//            OnUpdateMusic(static_cast<char *>(msg->GetObj()), msg->GetArg1());
//            break;
//
//        case kMusicDelete:
//            OnDeleteMusic(msg->GetArg1());
//            break;
        default:
            break;
    }
}

void VideoEditor::OnGLDestroy() {
    delete image_process_;
    image_process_ = nullptr;
}

void VideoEditor::OnAddAction(char *config, int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("add action id: %d config: %s", action_id, config);
    image_process_->OnAction(config, action_id);
    delete[] config;
}

void VideoEditor::OnUpdateAction(char *config, int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("update action id: %d config: %s", action_id, config);
    image_process_->OnAction(config, action_id);
    delete[] config;
}

void VideoEditor::OnDeleteAction(int action_id) {
    if (nullptr == image_process_) {
        return;
    }
    LOGI("delete action id: %d", action_id);
    image_process_->RemoveAction(action_id);
}

void VideoEditor::OnAddMusic(char *config, int action_id) {
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

void VideoEditor::OnUpdateMusic(char *config, int action_id) {
    if (nullptr != music_player_) {
        music_player_->Stop();
        music_player_->Start("");
    }
}

void VideoEditor::OnDeleteMusic(int aciton_id) {
    FreeMusicPlayer();
}

}  // namespace trinity
