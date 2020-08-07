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
//  player.h
//  player
//
//  Created by wlanjie on 2019/11/27.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef player_h
#define player_h

#include "music_decoder_controller.h"
#include "editor_resource.h"
#include "image_process.h"
#include "handler.h"
#include "egl_core.h"
#include "yuv_render.h"
#include "opengl.h"
#include "gl.h"
#include "buffer_pool.h"
#include "background.h"
#include "player_event_observer.h"

extern "C" {
#include "av_play.h"
#include "queue.h"
};

namespace trinity {

enum PlayerState {
    kNone,
    kStart,
    kResume,
    kPause,
    kStop
};

class Player : public Handler {
 public:
    Player(JNIEnv* env, jobject object);
    ~Player();

    int Init();
    /**
     * 在OpenGL线程发送消息,创建显示的surface
     * 并且创建OpenGL相关的资源, 包含屏幕显示, 硬解需要的纹理
     * 如果此时已经收到了开始播放或者暂停的消息, 则播放视频
     * @param window 创建EGLSurface需要的window
     */
    void OnSurfaceCreated(ANativeWindow* window);

    /**
     * SurfaceView改变时回调,记录新的宽和高
     * @param width 需要显示的宽
     * @param height 需要显示的高
     */
    void OnSurfaceChanged(int width, int height);

    /**
     * SurfaceView销毁时调用
     * 在OpenGL线程发送消息, 销毁EGLSurface
     * 同时把当前记录的视频的宽和高设置为0
     * 在渲染视频时需要重新建立一个FrameBuffer
     */
    void OnSurfaceDestroy();

    /**
     * 记录当前的MediaClip, 并且发送消息开始播放
     * @param clip 需要播放的视频信息
     * @param video_count_duration 当前播放前面几段的视频总合
     * @return 目前固定为0, 后面如果有error会以回调方式通知
     */
    int Start(MediaClip* clip, int video_count_duration = 0);

    /**
     * 预加载下一个视频
     * @param clip 预加载下一个视频的信息, 包含视频地址,开始结束时间
     * @return 0 成功, 其它为失败
     */
    int PreLoading(MediaClip* clip);

    /**
     * 快进快退
     * 注意: 这里会刷新视频帧到屏幕上
     * @param start_time 开始的时间
     * @param clip 要seek的片段
     * @param index 需要seek第几个
     */
    void Seek(int start_time, MediaClip* clp, int index);

    /**
     * 继续播放
     */
    void Resume();

    /**
     * 暂停播放
     */
    void Pause();

    /**
     * 停止播放, 并释放所有资源
     */
    void Stop();

    /**
     * 播放Play的所有对列和filter
     */
    void Destroy();

    /**
     * 获取当前播放的时间, 这里会累加前面片段的时间
     * @return 当前的时间
     */
    int64_t GetCurrentPosition();
    void AddObserver(PlayerEventObserver* observer);

    /**
     * 添加音乐
     * @param music_config 音乐的配置, 具体配置信息, 参考 TrinityVideoEditor.kt
     * @return 音乐对应的id, 后续的更新和删除都以这个id为准
     */
    int AddMusic(const char* music_config);

    /**
     * 更新音乐
     * @param music_config 音乐的配置, 具体配置信息, 参考 TrinityVideoEditor.kt
     * @param action_id 音乐对应的id, 在AddMusic时返回
     */
    void UpdateMusic(const char* music_config, int action_id);

    /**
     * 删除一个音乐
     * @param action_id 音乐对应的id, 在AddMusic时返回
     */
    void DeleteMusic(int action_id);

    /**
     * 添加一个滤镜
     * @param filter_config 滤镜配置, 具体配置信息, 参考 TrinityVideoEditor.kt
     * @return 滤镜对应的id, 后续的更新和删除都以这个id为准
     */
    int AddFilter(const char* filter_config);

    /**
     * 更新一个滤镜
     * @param filter_config 滤镜的配置, 具体配置信息, 参考 TrinityVideoEditor.kt
     * @param start_time 滤镜的开始时间
     * @param end_time 滤镜的结束时间
     * @param action_id 滤镜对应的id, 在AddFilter时返回
     */
    void UpdateFilter(const char* filter_config, int start_time, int end_time, int action_id);

    /**
     * 删除一个滤镜
     * @param action_id 滤镜对应的id, 在AddFilter时返回
     */
    void DeleteFilter(int action_id);

    /**
     * 添加一个特效
     * @param effect_config 特效配置
     * @return 特效对应的id, 后续的更新和删除都以这个id为准
     */
    int AddEffect(const char* effect_config);

    /**
     * 更新一个特效
     * @param start_time 特效的开始时间
     * @param end_time 特效的结束时间
     * @param action_id 特效对应的id, 在AddAction时返回
     */
    void UpdateEffect(int start_time, int end_time, int action_id);

    /**
     * 删除一个特效
     * @param action_id 特效对应的id, 在AddAction时返回
     */
    void DeleteEffect(int action_id);

    /**
     * 设置背景颜色
     * @param clip_index 应用的当前片段
     * @param red 红色 255
     * @param green 绿色 255
     * @param blue 蓝色 255
     * @param alpha 透明色
     */
    void SetBackgroundColor(int clip_index, int red, int green, int blue, int alpha);

    /**
     * 设置背景图片
     * @param clip_index 应用的当前片段
     * @param image_path 图片的地址
     */
    void SetBackgroundImage(int clip_index, const char* image_path);

    /**
     * 设置显示区域大小
     * @param width 需要显示的宽
     * @param height 需要显示的高
     */
    void SetFrameSize(int width, int height);

 private:
    AVPlayContext* CreatePlayContext(JNIEnv* env);
    int64_t GetVideoDuration();
    void OnAddAction(char* config, int action_id);
    void OnUpdateActionTime(int start_time, int end_time, int action_id);
    void OnDeleteAction(int action_id);
    void OnAddMusic(char* config, int action_id);
    void OnUpdateMusic(char* config, int action_id);
    void OnDeleteMusic(int action_id);
    void OnAddFilter(char* config, int action_id);
    void OnUpdateFilter(char* config, int start_time, int end_time, int action_id);
    void OnDeleteFilter(int action_id);
    void OnBackgroundColor(int clip_index, int red, int green, int blue, int alpha);
    void OnBackgroundImage(int clip_index, char* image_path);
    void FreeMusicPlayer();
    int GetAudioFrame();
    static int AudioCallback(uint8_t** buffer, int *buffer_size, void* context);
    static void OnComplete(AVPlayContext* context);
    static void OnStatusChanged(AVPlayContext* context, PlayStatus status);
    static void OnSeeking(AVPlayContext* context);
    void ReleasePlayContext();
    void ProcessMessage();
    static void* MessageQueueThread(void* args);
    virtual void HandleMessage(Message* msg);
    void CreateAudioRender();
    void OnStop();
    void OnRenderComplete();

    void SetFrame(int source_width, int source_height,
            int target_width, int target_height, RenderFrame frame_type = FIT);
    int OnDrawFrame(int texture_id, int width, int height);
    bool CheckVideoFrame();
    int DrawVideoFramePrepared();
    void CreateRenderFrameBuffer();
    void RenderFrameBuffer();
    void StartAudioRender();
    void PauseAudioRender();
    int DrawVideoFrame();
    void Draw(int texture_id);
    void ReleaseVideoFrame();
    void OnGLCreate();
    void OnGLWindowCreate();
    void OnRenderVideoFrame();
    void OnRenderImageFrame();
    void OnRenderSeekFrame();
    void OnGLWindowDestroy();
    void OnGLDestroy();

 private:
    pthread_t message_queue_thread_;
    AVPlayContext* av_play_context_;
    std::vector<AVPlayContext*> av_play_contexts_;
    int av_play_index_;
    MessageQueue* message_queue_;
    JavaVM* vm_;
    jobject object_;
    EGLCore* core_;
    YuvRender* yuv_render_;
    ImageProcess* image_process_;
    OpenGL* render_screen_;
    FrameBuffer* media_codec_render_;
    ANativeWindow* window_;
    EGLSurface render_surface_;
    int frame_width_;
    int frame_height_;
    int surface_width_;
    int surface_height_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    int current_action_id_;
    GLfloat* texture_matrix_;
    GLuint image_texture_;
    FrameBuffer* image_frame_buffer_;
    int image_render_count_;
    MusicDecoderController* music_player_;
    PlayerEventObserver* player_event_observer_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    bool window_created_;
    bool pre_loaded_;
    int seek_index_;
    MediaClip* pre_load_clip_;
    MediaClip* current_clip_;
    // 前几个视频的时间总和
    int video_count_duration_;
    AudioRender* audio_render_;
    bool audio_render_start_;
    int audio_buffer_size_;
    uint8_t* audio_buffer_;
    int draw_texture_id_;
    PlayerState player_state_;
    int64_t current_time_;
    int64_t previous_time_;
    BufferPool* buffer_pool_;
    Background* background_;
};

}  // namespace trinity

#endif /* player_h */
