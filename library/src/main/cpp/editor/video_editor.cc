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
    egl_destroy_ = false;
    egl_context_exists_ = false;
    video_state_ = nullptr;
    destroy_ = false;
    core_ = nullptr;
    render_surface_ = EGL_NO_SURFACE;
    yuv_render_ = nullptr;
    render_screen_ = nullptr;
    surface_width_ = 0;
    surface_height_ = 0;
    frame_width_ = 0;
    frame_height_ = 0;
    video_play_state_ = kNone;
    image_process_ = new ImageProcess();
    music_player_ = nullptr;
//    image_process_->AddSplitScreenAction(9, 0, INT64_MAX);

    message_queue_ = new MessageQueue("Video Render Message Queue");
    handler_ = new VideoRenderHandler(this, message_queue_);
    player_queue_ = new MessageQueue("Player Message Queue");
    player_handler_ = new PlayerHandler(this, player_queue_);
    audio_render_ = new AudioRender();

    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];

    vertex_coordinate_[0] = -1.0f;
    vertex_coordinate_[1] = -1.0f;
    vertex_coordinate_[2] = 1.0f;
    vertex_coordinate_[3] = -1.0f;

    vertex_coordinate_[4] = -1.0f;
    vertex_coordinate_[5] = 1.0f;
    vertex_coordinate_[6] = 1.0f;
    vertex_coordinate_[7] = 1.0f;

    texture_coordinate_[0] = 0.0f;
    texture_coordinate_[1] = 1.0f;
    texture_coordinate_[2] = 1.0f;
    texture_coordinate_[3] = 1.0f;

    texture_coordinate_[4] = 0.0f;
    texture_coordinate_[5] = 0.0f;
    texture_coordinate_[6] = 1.0f;
    texture_coordinate_[7] = 0.0f;
}

VideoEditor::~VideoEditor() {
    if (vertex_coordinate_ != nullptr) {
        delete[] vertex_coordinate_;
        vertex_coordinate_ = nullptr;
    }
    if (texture_coordinate_ != nullptr) {
        delete[] texture_coordinate_;
        texture_coordinate_ = nullptr;
    }
    if (nullptr != message_queue_) {
        delete message_queue_;
        message_queue_ = nullptr;
    }
    if (nullptr != handler_) {
        delete handler_;
        handler_ = nullptr;
    }
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
    result = pthread_create(&render_thread_, nullptr, RenderThread, this);
    if (result != 0) {
        LOGE("Init render thread error: %d", result);
        return false;
    }

    result = pthread_mutex_init(&render_mutex_, nullptr);
    if (result != 0) {
        LOGE("init render mutex error");
        return result;
    }
    result = pthread_cond_init(&render_cond_, nullptr);
    if (result != 0) {
        LOGE("init render cond error");
        return result;
    }

    result = pthread_create(&player_thread_, nullptr, PlayerThread, this);
    if (result != 0) {
        LOGE("Init player thread error: %d", result);
        return false;
    }
    return result;
}

void* VideoEditor::RenderThread(void *context) {
    VideoEditor* editor = (VideoEditor*) context;
    if (editor->destroy_) {
        return nullptr;
    }
    editor->ProcessMessage();
//    editor->ProcessRender();
    pthread_exit(0);
}

void VideoEditor::ProcessMessage() {
    bool rendering = true;
    while (rendering) {
        if (destroy_) {
            break;
        }
        Message *msg = NULL;
        if (message_queue_->DequeueMessage(&msg, true) > 0) {
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

void* VideoEditor::PlayerThread(void *context) {
    VideoEditor* editor = (VideoEditor*) context;
    if (editor->destroy_) {
        return nullptr;
    }
    editor->ProcessPlayerMessage();
    pthread_exit(0);
}

void VideoEditor::ProcessPlayerMessage() {
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

    if (nullptr != handler_) {
        if (!egl_context_exists_) {
            handler_->PostMessage(new Message(kCreateEGLContext, window_));
        } else {
            handler_->PostMessage(new Message(kCreateWindowSurface, window_));
            handler_->PostMessage(new Message(kRenderFrame));
        }
    }
}

void VideoEditor::OnSurfaceChanged(int width, int height) {
    surface_width_ = width;
    surface_height_ = height;
}

void VideoEditor::OnSurfaceDestroyed(JNIEnv* env) {
    LOGE("OnSurfaceDestroyed message_size: %d", handler_->GetQueueSize());
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(kDestroyWindowSurface));
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

int VideoEditor::Export(const char *path, int width, int height, int frame_rate, int video_bit_rate, int sample_rate,
                        int channel_count, int audio_bit_rate) {
    VideoExport* video_export = new VideoExport();
    return video_export->Export(clip_deque_, path, width, height, frame_rate, video_bit_rate, sample_rate, channel_count, audio_bit_rate);
}

int VideoEditor::Play(bool repeat, JNIEnv* env, jobject object) {
    if (clip_deque_.empty()) {
        return -1;
    }
    repeat_ = repeat;
    JavaVM* vm = nullptr;
    env->GetJavaVM(&vm);
    MediaClip* clip = clip_deque_.at(0);
    InitPlayer(clip);
    if (nullptr != audio_render_) {
        audio_render_->Play();
    }
    return 0;
}

void VideoEditor::Pause() {
    if (nullptr != video_state_ && (video_play_state_ == kResume || video_play_state_ == kPlaying)) {
        video_play_state_ = kPause;
        stream_toggle_pause(video_state_);
    }
}

void VideoEditor::Resume() {
    if (nullptr != video_state_ && video_play_state_ == kPause) {
        video_play_state_ = kResume;
        stream_toggle_pause(video_state_);
    }
}

void VideoEditor::Stop() {
}

void VideoEditor::Destroy() {
    LOGI("enter Destroy");
    destroy_ = true;
    handler_->PostMessage(new Message(kDestroyEGLContext));
    handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(render_thread_, nullptr);
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

    pthread_mutex_lock(&queue_mutex_);
    for (int i = 0; i < clip_deque_.size(); ++i) {
        MediaClip* clip = clip_deque_.at(i);
        delete[] clip->file_name;
        delete clip;
    }
    clip_deque_.clear();
    if (nullptr != audio_render_) {
        audio_render_->Stop();
    }
    pthread_mutex_unlock(&queue_mutex_);
    LOGE("start queue_mutex_ destroy");
    pthread_mutex_destroy(&queue_mutex_);
    pthread_cond_destroy(&queue_cond_);
    LOGE("leave Destroy");
}

static int AudioCallback(uint8_t* buffer, size_t buffer_size, void* context) {
    VideoEditor* editor = (VideoEditor*) context;
    return editor->ReadAudio(buffer, buffer_size);
}

static int OnCompleteState(StateContext *context) {
    VideoEditor* editor = (VideoEditor*) context->context;
    return editor->OnComplete();
}

int VideoEditor::OnComplete() {
    if (nullptr == video_state_) {
        return 1;
    }
    player_handler_->PostMessage(new Message(kStartPlayer));
    // 是否退出播放
    return 0;
}

void VideoEditor::StartPlayer() {
    LOGE("StartPlayer");
    video_state_->paused = 1;
    if (repeat_) {
        if (clip_deque_.size() == 1) {
            av_seek(video_state_, video_state_->start_time);
        } else {
            if (nullptr != audio_render_) {
                audio_render_->Stop();
            }
            av_destroy(video_state_);
            if (nullptr != video_state_) {
                av_freep(video_state_);
                video_state_ = nullptr;
            }

            play_index++;
            if (play_index >= clip_deque_.size()) {
                play_index = 0;
            }
            MediaClip* clip = clip_deque_.at(play_index);
            InitPlayer(clip);
        }
    } else {
        av_destroy(video_state_);
    }
    video_state_->paused = 0;
}

// Player method
int VideoEditor::InitPlayer(MediaClip* clip) {
    destroy_ = false;

    video_state_ = (VideoState*) av_malloc(sizeof(VideoState));
    video_state_->video_render_context = (VideoRenderContext*) av_malloc(sizeof(VideoRenderContext));
    video_state_->video_render_context->render_video_frame = SignalFrameAvailable;
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

bool VideoEditor::CreateEGLContext(ANativeWindow *window) {
    core_ = new EGLCore();
    bool result = core_->InitWithSharedContext();
    if (!result) {
        LOGE("Create EGLContext failed");
        return false;
    }
    CreateWindowSurface(window);
    core_->DoneCurrent();
    egl_context_exists_ = true;
    return result;
}

void VideoEditor::CreateWindowSurface(ANativeWindow *window) {
    LOGI("Enter CreateWindowSurface");
    if (core_ == nullptr || window == nullptr) {
        LOGE("CreateWindowSurface core_ is null");
        return;
    }
    render_surface_ = core_->CreateWindowSurface(window);
    if (render_surface_ != NULL && render_surface_ != EGL_NO_SURFACE) {
        core_->MakeCurrent(render_surface_);

        render_screen_ = new OpenGL(surface_width_, surface_height_, DEFAULT_VERTEX_MATRIX_SHADER, DEFAULT_FRAGMENT_SHADER);
//        image_process_->AddFlashWhiteAction(2000, 0, 3000);
    }
    LOGE("Leave CreateWindowSurface");
}

void VideoEditor::DestroyWindowSurface() {
    LOGE("enter DestroyWindowSurface");
    if (EGL_NO_SURFACE != render_surface_) {
        if (nullptr != yuv_render_) {
            delete yuv_render_;
            yuv_render_ = nullptr;
        }
        if (nullptr != render_screen_) {
            delete render_screen_;
            render_screen_ = nullptr;
        }
        if (nullptr != core_) {
            core_->ReleaseSurface(render_surface_);
        }
        render_surface_ = EGL_NO_SURFACE;
        if (nullptr != window_) {
            ANativeWindow_release(window_);
            window_ = nullptr;
        }
    }
    LOGE("Leave DestroyWindowSurface");
}

void VideoEditor::DestroyEGLContext() {
    LOGI("enter DestroyEGLContext");
    if (EGL_NO_SURFACE != render_surface_) {
        core_->MakeCurrent(render_surface_);
    }
    DestroyWindowSurface();
    if (nullptr != core_) {
        core_->Release();
        delete core_;
        core_ = nullptr;
    }
    egl_context_exists_ = false;
    egl_destroy_ = true;
    LOGI("leave DestroyEGLContext");
}

int VideoEditor::ReadAudio(uint8_t *buffer, int buffer_size) {
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

void VideoEditor::RenderVideo() {
    if (nullptr != core_ && EGL_NO_SURFACE != render_surface_) {
        Frame *vp = frame_queue_peek_last(&video_state_->video_queue);
        if (! core_->MakeCurrent(render_surface_)) {
            LOGE("eglSwapBuffers MakeCurrent error: %d", eglGetError());
        }
        if (!vp->uploaded) {
            int width = MIN(vp->frame->linesize[0], vp->frame->width);
            int height = vp->frame->height;
            if (frame_width_ != width || frame_height_ != height) {
                frame_width_ = width;
                frame_height_ = height;
                if (nullptr != yuv_render_) {
                    delete yuv_render_;
                }
                yuv_render_ = new YuvRender(vp->width, vp->height, 0);
            }
            int texture_id = yuv_render_->DrawFrame(vp->frame);
            uint64_t current_time = (uint64_t) (vp->frame->pts * av_q2d(video_state_->video_st->time_base) * 1000);
            texture_id = image_process_->Process(texture_id, current_time, vp->width, vp->height, 0, 0);
            render_screen_->ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_);
            if (!core_->SwapBuffers(render_surface_)) {
                LOGE("eglSwapBuffers error: %d", eglGetError());
            }
            pthread_mutex_lock(&render_mutex_);
            if (handler_->GetQueueSize() <= 1) {
                pthread_cond_signal(&render_cond_);
            }
            pthread_mutex_unlock(&render_mutex_);
            vp->uploaded = 1;
        }
    }
}

void VideoEditor::SignalFrameAvailable(VideoRenderContext* context) {
    if (nullptr != context && nullptr != context->context) {
        VideoEditor* editor = (VideoEditor*) context->context;
        if (!editor->egl_context_exists_) {
            return;
        }
        editor->video_play_state_ = kPlaying;
        VideoRenderHandler* handler = editor->handler_;
        if (nullptr != handler) {
            pthread_mutex_lock(&editor->render_mutex_);
            if (handler->GetQueueSize() > 1) {
                pthread_cond_wait(&editor->render_cond_, &editor->render_mutex_);
            }
            pthread_mutex_unlock(&editor->render_mutex_);
            handler->PostMessage(new Message(kRenderFrame));
        }
    }
}

}