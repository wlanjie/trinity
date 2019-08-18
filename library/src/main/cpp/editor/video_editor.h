//
// Created by wlanjie on 2019-05-14.
//

#ifndef TRINITY_VIDEO_EDITOR_H
#define TRINITY_VIDEO_EDITOR_H

#include <cstdint>
#include <jni.h>
#include <android/native_window_jni.h>
#include <deque>
#include <pthread.h>

#include "video_player.h"
#include "yuv_render.h"
#include "pixel_late.h"
#include "flash_white.h"
#include "egl_core.h"
#include "handler.h"
#include "opengl.h"
#include "image_process.h"
#include "music_decoder_controller.h"
#include "edtior_resource.h"
#include "trinity.h"

using namespace std;

namespace trinity {

class VideoEditor : public GLObserver {
public:
    VideoEditor(const char* resource_path);
    ~VideoEditor();

    int Init();

    void OnSurfaceCreated(JNIEnv* env, jobject object, jobject surface);

    void OnSurfaceChanged(int width, int height);

    void OnSurfaceDestroyed(JNIEnv* env);

    // 获取所有clip的时长总和
    int64_t GetVideoDuration() const;

    int64_t GetCurrentPosition() const;

    // 获取所有clip的数量
    int GetClipsCount();

    // 根据index获取clip
    MediaClip* GetClip(int index);

    // 插入一段clip
    int InsertClip(MediaClip* clip);

    // 根据index插入一段clip
    int InsertClipWithIndex(int index, MediaClip* clip);

    // 根据index删除一段clip
    void RemoveClip(int index);

    // 替换index所在的clip
    int ReplaceClip(int index, MediaClip* clip);

    // 获取所有的clip

    // 获取 TimeRange

    //
    int64_t GetVideoTime(int index, int64_t clip_time);

    //
    int64_t GetClipTime(int index, int64_t video_time);

    // 根据时间获取clip的index
    int GetClipIndex(int64_t time);

    // 添加滤镜
    int AddFilter(const char* lut, uint64_t start_time, uint64_t end_time, int action_id);

    int AddMusic(const char* path, uint64_t start_time, uint64_t end_time);

    int AddAction(int effect_type, uint64_t start_time, uint64_t end_time, int action_id = -1);

    // 开始播放
    // 是否循环播放
    int Play(bool repeat, JNIEnv* env, jobject object);

    // 暂停播放
    void Pause();

    // 继续播放
    void Resume();

    // 停止播放
    void Stop();

    void Destroy();

    int OnComplete();

    virtual void OnGLCreate();

    virtual void OnGLMessage(Message* msg);

    virtual void OnGLDestroy();
private:
    void OnFilter(FilterAction* action);
    void OnFlashWhite(FlashWhiteAction* action);
private:
    static int OnCompleteEvent(StateEvent* event);
    static int OnVideoRender(OnVideoRenderEvent* event, int texture_id, int width, int height, uint64_t current_time);
    void FreeStateEvent();
    void AllocStateEvent();
    void FreeVideoRenderEvent();
    void AllocVideoRenderEvent();
    static void* CompleteThread(void* context);
    void ProcessMessage();
private:
    EditorResource* editor_resource_;
    deque<MediaClip*> clip_deque_;
    pthread_mutex_t queue_mutex_;
    pthread_cond_t queue_cond_;
    ANativeWindow* window_;
    jobject video_editor_object_;

    VideoPlayer* video_player_;
    // 是否循环播放
    bool repeat_;
    // 当前播放的文件位置
    int play_index;
    ImageProcess* image_process_;

    MusicDecoderController* music_player_;
    StateEvent* state_event_;
    OnVideoRenderEvent* on_video_render_event_;

    pthread_t complete_thread_;
    MessageQueue* message_queue_;
    PlayerHandler* handler_;

    int current_action_id_;
};

class PlayerHandler : public Handler {
public:
    PlayerHandler(VideoEditor* editor, MessageQueue* queue) : Handler(queue) {
        editor_ = editor;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        switch (what) {
            case kStartPlayer:
                editor_->OnComplete();
                break;

            default:
                break;
        }
    }
private:
    VideoEditor* editor_;
};

}

#endif //TRINITY_VIDEO_EDITOR_H
