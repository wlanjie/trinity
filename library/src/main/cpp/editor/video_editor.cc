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

VideoEditor::VideoEditor(JNIEnv* env, jobject object, const char* resource_path) : Handler()
    , window_(nullptr)
    , video_editor_object_()
    , repeat_(false)
    , play_index(0)
    , editor_resource_()
    , vm_()
    , queue_mutex_()
    , queue_cond_() {
    window_ = nullptr;
    repeat_ = false;
    play_index = 0;
    player_ = new Player(env, object);
    player_->AddObserver(this);
    editor_resource_ = new EditorResource(resource_path);
    env->GetJavaVM(&vm_);
    video_editor_object_ = env->NewGlobalRef(object);
}

VideoEditor::~VideoEditor() {
    if (nullptr != player_) {
        player_->Destroy();
        delete player_;
        player_ = nullptr;
    }
    if (nullptr != editor_resource_) {
        delete editor_resource_;
        editor_resource_ = nullptr;
    }
    JNIEnv* env = nullptr;
    if ((vm_)->GetEnv((void**) &env, JNI_VERSION_1_6) == JNI_OK) {
        env->DeleteGlobalRef(video_editor_object_);
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

void VideoEditor::OnSurfaceCreated(jobject surface) {
    if (nullptr != window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
    JNIEnv* env = nullptr;
    if ((vm_)->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return;
    }
    window_ = ANativeWindow_fromSurface(env, surface);
    if (nullptr != player_) {
        player_->OnSurfaceCreated(window_);
    }
}

void VideoEditor::OnSurfaceChanged(int width, int height) {
    if (nullptr != player_) {
        player_->OnSurfaceChanged(width, height);
    }
}

void VideoEditor::OnSurfaceDestroyed() {
    if (nullptr != player_) {
        player_->OnSurfaceDestroy();
    }
    if (nullptr != window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
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
    if (nullptr != player_) {
        return player_->GetCurrentPosition();
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

int VideoEditor::AddMusic(const char* music_config) {
    if (nullptr != player_) {
        int action_id = player_->AddMusic(music_config);
        if (nullptr != editor_resource_) {
            editor_resource_->AddMusic(music_config, action_id);
        }
        return action_id;
    }
    return -1;
}

void VideoEditor::UpdateMusic(const char* music_config, int action_id) {
    if (nullptr != player_) {
        player_->UpdateMusic(music_config, action_id);

        if (nullptr != editor_resource_) {
            editor_resource_->UpdateMusic(music_config, action_id);
        }
    }
}

void VideoEditor::DeleteMusic(int action_id) {
    if (nullptr != player_) {
        player_->DeleteMusic(action_id);
        if (nullptr != editor_resource_) {
            editor_resource_->DeleteMusic(action_id);
        }
    }
}

int VideoEditor::AddAction(const char *effect_config) {
    if (nullptr != player_) {
        auto action_id = player_->AddAction(effect_config);
        if (nullptr != editor_resource_) {
            editor_resource_->AddAction(effect_config, action_id);
        }
        return action_id;
    }
    return -1;
}

void VideoEditor::UpdateAction(const char *effect_config, int action_id) {
    if (nullptr != player_) {
        player_->UpdateAction(effect_config, action_id);
        if (nullptr != editor_resource_) {
            editor_resource_->UpdateAction(effect_config, action_id);
        }
    }
}

void VideoEditor::DeleteAction(int action_id) {
    if (nullptr != player_) {
        player_->DeleteAction(action_id);
    }
    if (nullptr != editor_resource_) {
        editor_resource_->DeleteAction(action_id);
    }
}

void VideoEditor::OnComplete() {
    LOGE("enter %s", __func__);
    if (clip_deque_.size() == 1) {
        play_index = repeat_ ? 0 : -1;
    } else {
        play_index++;
        if (play_index >= clip_deque_.size()) {
            play_index = repeat_ ? 0 : -1;
        }
    }
    if (play_index == -1) {
        return;
    }

    MediaClip* clip = clip_deque_.at(play_index);
    if (nullptr != player_) {
        player_->Start(clip->file_name, clip->start_time, clip->end_time);
    }
    LOGE("leave %s", __func__);
}

void VideoEditor::Seek(int time) {
    if (nullptr != player_) {
        player_->Seek(time, INT_MAX);
    }
}

int VideoEditor::Play(bool repeat, JNIEnv* env, jobject object) {
    if (clip_deque_.empty()) {
        return -1;
    }
    repeat_ = repeat;
    MediaClip* clip = clip_deque_.at(0);

    if (nullptr != player_) {
        return player_->Start(clip->file_name, clip->start_time, clip->end_time);
    }
    return 0;
}

void VideoEditor::Pause() {
    if (nullptr != player_) {
        player_->Pause();
    }
}

void VideoEditor::Resume() {
    if (nullptr != player_) {
        player_->Resume();
    }
}

void VideoEditor::Stop() {}

void VideoEditor::Destroy() {
    LOGI("enter Destroy");
    if (nullptr != player_) {
        player_->Destroy();
    }

    pthread_mutex_lock(&queue_mutex_);
    for (int i = 0; i < clip_deque_.size(); ++i) {
        MediaClip* clip = clip_deque_.at(i);
        delete[] clip->file_name;
        delete clip;
    }
    clip_deque_.clear();
    pthread_mutex_unlock(&queue_mutex_);
    pthread_mutex_destroy(&queue_mutex_);
    pthread_cond_destroy(&queue_cond_);
    LOGI("leave Destroy");
}

}  // namespace trinity
