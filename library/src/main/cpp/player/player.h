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
    void OnSurfaceCreated(ANativeWindow* window);
    void OnSurfaceChanged(int width, int height);
    void OnSurfaceDestroy();
    int Start(MediaClip* clip, int video_count_duration = 0);
    void Seek(int start_time, int end_time);
    void Resume();
    void Pause();
    void Stop();
    void Destroy();
    int64_t GetCurrentPosition();
    void AddObserver(PlayerEventObserver* observer);
    int AddMusic(const char* music_config);
    void UpdateMusic(const char* music_config, int action_id);
    void DeleteMusic(int action_id);
    int AddFilter(const char* filter_config);
    void UpdateFilter(const char* filter_config, int start_time, int end_time, int action_id);
    void DeleteFilter(int action_id);
    int AddAction(const char* effect_config);
    void UpdateAction(int start_time, int end_time, int action_id);
    void DeleteAction(int action_id);

 private:
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
    void FreeMusicPlayer();
    int GetAudioFrame();
    static int AudioCallback(uint8_t** buffer, int *buffer_size, void* context);
    static void OnComplete(AVPlayContext* context);
    static void OnStatusChanged(AVPlayContext* context, PlayStatus status);
    void ReleasePlayContext();
    void ProcessMessage();
    static void* MessageQueueThread(void* args);
    virtual void HandleMessage(Message* msg);

    void SetFrame(int source_width, int source_height,
            int target_width, int target_height, RenderFrame frame_type = FIT);
    int OnDrawFrame(int texture_id, int width, int height);
    int DrawVideoFrame();
    void Draw(int texture_id);
    void ReleaseVideoFrame();
    void OnGLCreate();
    void OnGLWindowCreate();
    void OnRenderVideoFrame();
    void OnRenderImageFrame();
    void OnGLWindowDestroy();
    void OnGLDestroy();

 private:
    pthread_t message_queue_thread_;
    AVPlayContext* av_play_context_;
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
    GLuint oes_texture_;
    GLuint image_texture_;
    FrameBuffer* image_frame_buffer_;
    int64_t image_render_start_time_;
    MusicDecoderController* music_player_;
    PlayerEventObserver* player_event_observer_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    bool window_created_;
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
};

}  // namespace trinity

#endif /* player_h */
