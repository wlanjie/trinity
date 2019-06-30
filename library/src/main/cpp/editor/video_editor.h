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
#include "video_export.h"

using namespace std;

namespace trinity {

class VideoEditor {
public:
    VideoEditor();
    ~VideoEditor();

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

    int Export(const char* export_config, const char* path, int width, int height, int frame_rate, int video_bit_rate,
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

    void RenderVideo();

//    static void StartPlayer(PlayerActionContext* context);

//    static void RenderVideoFrame(PlayerActionContext* context);

    int OnComplete();

    bool egl_destroy_;

private:
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
    int video_play_state_;
    bool egl_context_exists_;
    bool destroy_;
    ImageProcess* image_process_;

    MusicDecoderController* music_player_;
};

}

#endif //TRINITY_VIDEO_EDITOR_H
