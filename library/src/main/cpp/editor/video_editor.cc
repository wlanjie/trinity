//
// Created by wlanjie on 2019-05-14.
//

#include "video_editor.h"
#include "android_xlog.h"
#include "gl.h"

namespace trinity {

VideoEditor::VideoEditor() {
    window_ = nullptr;
    video_editor_object_ = nullptr;
    repeat_ = false;
    play_index = 0;
    video_player_ = new VideoPlayer();
    image_process_ = new ImageProcess();
    music_player_ = nullptr;
    state_event_ = nullptr;
}

VideoEditor::~VideoEditor() {
    if (nullptr != video_player_) {
        delete video_player_;
        video_player_ = nullptr;
    }
    if (nullptr != image_process_) {
        delete image_process_;
        image_process_ = nullptr;
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
    if (nullptr != video_player_) {
        video_player_->Init();
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

    if (nullptr != video_player_) {
        video_player_->OnSurfaceCreated(window_);
    }
}

void VideoEditor::OnSurfaceChanged(int width, int height) {
    if (nullptr != video_player_) {
        video_player_->OnSurfaceChanged(width, height);
    }
}

void VideoEditor::OnSurfaceDestroyed(JNIEnv* env) {
    if (nullptr != video_player_) {
        video_player_->OnSurfaceDestroyed();
    }
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
    MediaClip* clip = clip_deque_.at(index);
    delete clip;
    clip_deque_.erase(clip_deque_.begin() + index);
}

int VideoEditor::ReplaceClip(int index, MediaClip *clip) {
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

int VideoEditor::AddFilter(uint8_t *lut, int lut_size, uint64_t start_time, uint64_t end_time, int action_id) {
    if (nullptr != image_process_) {
        return image_process_->AddFilterAction(lut, lut_size, start_time, end_time, action_id);
    }
    return 0;
}

int VideoEditor::AddMusic(const char *path, uint64_t start_time, uint64_t end_time) {
    if (nullptr == music_player_) {
        music_player_ = new MusicDecoderController();
        music_player_->Init(0.2f, 44100);
        music_player_->Start(path);
    }
    return 0;
}

int VideoEditor::Export(const char* export_config, const char *path, int width, int height, int frame_rate, int video_bit_rate, int sample_rate,
                        int channel_count, int audio_bit_rate) {
    VideoExport* video_export = new VideoExport();
    return video_export->Export(export_config, path, width, height, frame_rate, video_bit_rate, sample_rate, channel_count, audio_bit_rate);
}

int VideoEditor::OnCompleteEvent(StateEvent *event) {
    VideoEditor *editor = (VideoEditor *)event->context;
    if (nullptr != editor) {
        return editor->OnComplete();
    }
    return 0;
}

int VideoEditor::OnComplete() {
    if (repeat_) {
        if (clip_deque_.size() == 1) {
            MediaClip* clip = clip_deque_.at(0);
            video_player_->Seek(clip->start_time);
        } else {
            video_player_->Stop();
            play_index++;
            if (play_index >= clip_deque_.size()) {
                play_index = 0;
            }
            MediaClip* clip = clip_deque_.at(play_index);
            FreeStateEvent();
            video_player_->Start(clip->file_name, clip->start_time, clip->end_time == 0 ? INT64_MAX : clip->end_time, state_event_);
        }
    } else {
        video_player_->Stop();
    }
    return 0;
}

void VideoEditor::FreeStateEvent() {
    if (nullptr != state_event_) {
        av_freep(state_event_);
    }
}

void VideoEditor::AllocStateEvent() {
    state_event_ = (StateEvent*) av_malloc(sizeof(StateEvent));
    memset(state_event_, 0, sizeof(StateEvent));
    state_event_->on_complete_event = OnCompleteEvent;
    state_event_->context = this;
}

int VideoEditor::Play(bool repeat, JNIEnv* env, jobject object) {
    if (clip_deque_.empty()) {
        return -1;
    }
    FreeStateEvent();
    AllocStateEvent();
    repeat_ = repeat;
    MediaClip* clip = clip_deque_.at(0);
    video_player_->Start(clip->file_name, clip->start_time, clip->end_time == 0 ? INT64_MAX : clip->end_time, state_event_);
    return 0;
}

void VideoEditor::Pause() {
    if (nullptr != video_player_) {
        video_player_->Pause();
    }
}

void VideoEditor::Resume() {
    if (nullptr != video_player_) {
        video_player_->Resume();
    }
}

void VideoEditor::Stop() {

}

void VideoEditor::Destroy() {
    LOGI("enter Destroy");
    if (nullptr != video_player_) {
        video_player_->Stop();
        video_player_->Destroy();
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
    LOGE("leave Destroy");
}

}