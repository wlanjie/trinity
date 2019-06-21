//
// Created by wlanjie on 2019-05-14.
//

#ifndef TRINITY_VIDEO_EDITOR_H
#define TRINITY_VIDEO_EDITOR_H

extern "C" {
#include "ffplay.h"
};

#include <cstdint>
#include <jni.h>
#include <android/native_window_jni.h>
#include <deque>
#include <pthread.h>

#include "player.h"
#include "yuv_render.h"
#include "pixel_late.h"
#include "flash_white.h"
#include "egl_core.h"
#include "handler.h"
#include "opengl.h"
#include "image_process.h"
#include "music_decoder_controller.h"
#include "video_export.h"

using namespace std;

namespace trinity {

class VideoRenderHandler;

typedef enum {
    kCreateEGLContext,
    kCreateWindowSurface,
    kDestroyWindowSurface,
    kDestroyEGLContext,
    kRenderFrame
} VideoRenderMessage;

class VideoEditor {
public:
    VideoEditor();
    ~VideoEditor();

    void testTranscode();

    int Init();

    void OnSurfaceCreated(JNIEnv* env, jobject object, jobject surface);

    void OnSurfaceChanged(int width, int height);

    void OnSurfaceDestroyed(JNIEnv* env);

    // 获取所有clip的时长总和
    int64_t GetVideoDuration() const;

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
    int AddFilter(uint8_t* lut, int lut_size, uint64_t start_time, uint64_t end_time, int action_id);

    int AddMusic(const char* path, uint64_t start_time, uint64_t end_time);

    int Export(const char* path, int width, int height, int frame_rate, int video_bit_rate,
            int sample_rate, int channel_count, int audio_bit_rate);

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

public:

    bool CreateEGLContext(ANativeWindow* window);

    void CreateWindowSurface(ANativeWindow* window);

    void DestroyWindowSurface();

    void DestroyEGLContext();

    void RenderVideo();

    static void StartPlayer(PlayerActionContext* context);

    static void RenderVideoFrame(PlayerActionContext* context);

    int OnComplete();

    bool egl_destroy_;

private:
    static void *RenderThread(void *context);

    void ProcessMessage();

private:
    deque<MediaClip*> clip_deque_;
    pthread_mutex_t queue_mutex_;
    pthread_cond_t queue_cond_;
    ANativeWindow* window_;
    jobject video_editor_object_;

    Player* player_;
    // 是否循环播放
    bool repeat_;
    // 当前播放的文件位置
    int play_index;
    int video_play_state_;
    pthread_t render_thread_;
    pthread_mutex_t render_mutex_;
    pthread_cond_t render_cond_;
    bool egl_context_exists_;
    bool destroy_;
    EGLCore* core_;
    EGLSurface render_surface_;
    VideoRenderHandler* handler_;
    MessageQueue* message_queue_;
    YuvRender* yuv_render_;
    OpenGL* render_screen_;
    int surface_width_;
    int surface_height_;
    int frame_width_;
    int frame_height_;
    ImageProcess* image_process_;

    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;

    MusicDecoderController* music_player_;
};

class VideoRenderHandler : public Handler {
public:
    VideoRenderHandler(VideoEditor* editor, MessageQueue* queue) : Handler(queue) {
        editor_ = editor;
        init_ = false;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        ANativeWindow* window;
        switch (what) {
            case kCreateEGLContext:
                if (editor_->egl_destroy_) {
                    break;
                }
                window = (ANativeWindow *) (msg->GetObj());
                init_ = editor_->CreateEGLContext(window);
                break;

            case kRenderFrame:
                if (editor_->egl_destroy_) {
                    break;
                }
                if (init_) {
                    editor_->RenderVideo();
                }
                break;

            case kCreateWindowSurface:
                if (editor_->egl_destroy_) {
                    break;
                }
                if (init_) {
                    window = (ANativeWindow *) (msg->GetObj());
                    editor_->CreateWindowSurface(window);
                }
                break;

            case kDestroyWindowSurface:
                if (init_) {
                    editor_->DestroyWindowSurface();
                }
                break;

            case kDestroyEGLContext:
                editor_->DestroyEGLContext();
                break;

            default:
                break;
        }
    }

private:
    VideoEditor* editor_;
    bool init_;
};

}

#endif //TRINITY_VIDEO_EDITOR_H
