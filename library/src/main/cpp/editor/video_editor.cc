//
// Created by wlanjie on 2019-05-14.
//

#include "video_editor.h"
#include "android_xlog.h"
#include "gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace trinity {

VideoEditor::VideoEditor(const char* resource_path) : current_action_id_(0) {
    window_ = nullptr;
    video_editor_object_ = nullptr;
    repeat_ = false;
    play_index = 0;
    video_player_ = new VideoPlayer();
    video_player_->RegisterVideoFrameObserver(this);
    editor_resource_ = new EditorResource(resource_path);
    music_player_ = nullptr;
    state_event_ = nullptr;
    on_video_render_event_ = nullptr;

    message_queue_ = new MessageQueue("Video Complete Message Queue");
    handler_ = new PlayerHandler(this, message_queue_);
}

VideoEditor::~VideoEditor() {
    if (nullptr != video_player_) {
        delete video_player_;
        video_player_ = nullptr;
    }
    if (nullptr != editor_resource_) {
        delete editor_resource_;
        editor_resource_ = nullptr;
    }
    if (nullptr != message_queue_) {
        message_queue_->Abort();
        delete message_queue_;
        message_queue_ = nullptr;
    }
    if (nullptr != handler_) {
        delete handler_;
        handler_ = nullptr;
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
    result = pthread_create(&complete_thread_, nullptr, CompleteThread, this);
    if (result != 0) {
        LOGE("Init complete thread error: %d", result);
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

int64_t VideoEditor::GetCurrentPosition() const {
    if (nullptr != video_player_) {
        return video_player_->GetCurrentPosition();
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

int VideoEditor::AddFilter(const char* lut, uint64_t start_time, uint64_t end_time, int action_id) {
    int actId = action_id;
    if (action_id == NO_ACTION) {
        actId = current_action_id_++;
    }
    if (nullptr != video_player_) {
        char* lut_path = new char[strlen(lut) + 1];
        sprintf(lut_path, "%s%c", lut, 0);
        FilterAction* action = new FilterAction();
        action->start_time = start_time;
        action->end_time = end_time;
        action->action_id = actId;
        action->lut_path = lut_path;
        Message* message = new Message(kFilter, action);
        video_player_->SendGLMessage(message);
    }
    return actId;
}

int VideoEditor::AddMusic(const char *path, uint64_t start_time, uint64_t end_time) {
    if (nullptr == music_player_) {
        music_player_ = new MusicDecoderController();
        music_player_->Init(0.2f, 44100);
        music_player_->Start(path);
    }
    return 0;
}

int VideoEditor::AddAction(int effect_type, uint64_t start_time, uint64_t end_time, int action_id) {
    int actId = action_id;
    if (action_id == NO_ACTION) {
        actId = current_action_id_++;
    }
    if (nullptr != video_player_) {
        if (effect_type == 0) {
            FlashWhiteAction *action = new FlashWhiteAction();
            action->start_time = start_time;
            action->end_time = end_time;
            action->flash_time = 10;
            action->action_id = actId;
            Message *msg = new Message(kFlashWhite, action);
            video_player_->SendGLMessage(msg);
        }
    }
    return actId;
}

int VideoEditor::OnCompleteEvent(StateEvent *event) {
    VideoEditor *editor = (VideoEditor *)event->context;
    if (nullptr != editor) {
        editor->handler_->PostMessage(new Message(kStartPlayer));
//        return editor->OnComplete();
    }
    return 0;
}

int VideoEditor::OnVideoRender(OnVideoRenderEvent *event, int texture_id, int width, int height, uint64_t current_time) {
    VideoEditor* editor = (VideoEditor*) event->context;
    if (nullptr != editor) {
        return editor->image_process_->Process(texture_id, current_time, width, height, 0, 0);
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
            AllocStateEvent();
            video_player_->Start(clip->file_name, clip->start_time,
                    clip->end_time == 0 ? INT64_MAX : clip->end_time, state_event_, on_video_render_event_);
        }
    } else {
        video_player_->Stop();
    }
    return 0;
}

void VideoEditor::FreeStateEvent() {
    if (nullptr != state_event_) {
        av_free(state_event_);
        state_event_ = nullptr;
    }
}

void VideoEditor::AllocStateEvent() {
    state_event_ = (StateEvent*) av_malloc(sizeof(StateEvent));
    memset(state_event_, 0, sizeof(StateEvent));
    state_event_->on_complete_event = OnCompleteEvent;
    state_event_->context = this;
}

void VideoEditor::FreeVideoRenderEvent() {
    if (nullptr != on_video_render_event_) {
        av_free(on_video_render_event_);
        on_video_render_event_ = nullptr;
    }
}

void VideoEditor::AllocVideoRenderEvent() {
    on_video_render_event_ = (OnVideoRenderEvent*) av_malloc(sizeof(OnVideoRenderEvent));
    memset(on_video_render_event_, 0, sizeof(OnVideoRenderEvent));
    on_video_render_event_->on_render_video = OnVideoRender;
    on_video_render_event_->context = this;
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
    video_player_->Start(clip->file_name,
            clip->start_time, clip->end_time == 0 ? INT64_MAX : clip->end_time,
            state_event_, on_video_render_event_);
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
    FreeStateEvent();
    FreeVideoRenderEvent();
    handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(complete_thread_, nullptr);
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
        case kFilter:
            OnFilter(static_cast<FilterAction *>(msg->GetObj()));
            break;

        case kFlashWhite:
            OnFlashWhite(static_cast<FlashWhiteAction *>(msg->GetObj()));
            break;
        default:
            break;
    }
}

void VideoEditor::OnGLDestroy() {
    delete image_process_;
    image_process_ = nullptr;
}

void VideoEditor::OnFilter(FilterAction *action) {
    if (nullptr != image_process_) {
        int lut_width = 0;
        int lut_height = 0;
        int channels = 0;
        unsigned char* lut_buffer = stbi_load(action->lut_path, &lut_width, &lut_height, &channels, STBI_rgb_alpha);
        if (lut_width != 512 || lut_height != 512) {
            stbi_image_free(lut_buffer);
            return;
        }
        int actionId = image_process_->AddFilterAction(lut_buffer, action->start_time, action->end_time, action->action_id);
        stbi_image_free(lut_buffer);

        if (nullptr != editor_resource_) {
            editor_resource_->AddFilter(action->lut_path, action->start_time, action->end_time, actionId);
        }
        delete[] action->lut_path;
        delete action;
    }
}

void VideoEditor::OnFlashWhite(FlashWhiteAction *action) {
    if (nullptr != image_process_) {
        image_process_->AddFlashWhiteAction(action->flash_time, action->start_time, action->end_time, action->action_id);
        delete[] action;
    }
}

}