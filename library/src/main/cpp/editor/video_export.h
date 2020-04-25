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
// Created by wlanjie on 2019-06-15.
//

#ifndef TRINITY_VIDEO_EXPORT_H
#define TRINITY_VIDEO_EXPORT_H

#include <jni.h>

#include <deque>
#include <fstream>
#include <iostream>

#include "video_encoder_adapter.h"
#include "audio_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "music_decoder.h"
#include "decode/resample.h"
#include "yuv_render.h"
#include "image_process.h"
#include "handler.h"
#include "trinity.h"

extern "C" {
#include "av_play.h"
#include "queue.h"
#include "cJSON.h"
};

namespace trinity {

enum {
    kStartNextExport
};

class VideoExportHandler;

class VideoExport {
 public:
    VideoExport(JNIEnv* env, jobject object);
    ~VideoExport();

    int Export(const char* export_config, const char* path,
            int width, int height, int frame_rate, int video_bit_rate,
            int sample_rate, int channel_count, int audio_bit_rate,
            bool media_codec_decode, bool media_codec_encode);

    void Cancel();
 private:
    void CreateEncode(bool media_codec_encode);
    static void OnCompleteEvent(AVPlayContext* context);
    static void OnStatusChanged(AVPlayContext* context, PlayStatus status);
    int OnComplete();
    static void* ExportVideoThread(void* context);
    static void* ExportAudioThread(void* context);
    void StartDecode(MediaClip* clip);
    void LoadImageTexture(MediaClip* clip);
    void FreeResource();
    void OnFilter();
    void OnEffect();
    void OnMusics();
    void SetFrame(int source_width, int source_height,
            int target_width, int target_height, RenderFrame frame_type);
    void ProcessVideoExport();
    void ProcessAudioExport();
    void OnExportProgress(int64_t current_time);
    void OnExportComplete();
    void OnCancel();
    int Resample();
    int FillMuteAudio();

 private:
    JavaVM* vm_;
    jobject object_;
    int64_t video_duration_;
    pthread_t export_video_thread_;
    pthread_t export_audio_thread_;
    MediaClip* current_media_clip_;
    std::deque<MediaClip*> clip_deque_;
    std::deque<MusicDecoder*> music_decoder_deque_;
    std::deque<trinity::Resample*> resample_deque_;
    int accompany_packet_buffer_size_;
    int accompany_sample_rate_;
    int vocal_sample_rate_;
    int channel_count_;
    bool export_ing_;
    bool cancel_;
    EGLCore* egl_core_;
    EGLSurface egl_surface_;
    GLuint encode_texture_id_;
    GLuint image_texture_;
    int64_t image_render_time_;
    bool load_image_texture_;
    int export_index_;
    int video_width_;
    int video_height_;
    int frame_rate_;
    int video_bit_rate_;
    int frame_width_;
    int frame_height_;
    YuvRender* yuv_render_;
    ImageProcess* image_process_;
    bool media_codec_encode_;
    VideoEncoderAdapter* encoder_;
    AudioEncoderAdapter* audio_encoder_adapter_;
    VideoConsumerThread* packet_thread_;
    PacketPool* packet_pool_;
    int64_t current_time_;
    int64_t previous_time_;
    SwrContext* swr_context_;
    uint8_t *audio_buffer_;
    int audio_current_time_;
    uint8_t *audio_buf1;
    short* audio_samples_;
    pthread_mutex_t media_mutex_;
    pthread_cond_t media_cond_;
    pthread_mutex_t audio_mutex_;
    pthread_cond_t audio_cond_;
    cJSON* export_config_json_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    GLfloat* crop_vertex_coordinate_;
    GLfloat* crop_texture_coordinate_;
    GLfloat* texture_matrix_;
    AVPlayContext* av_play_context_;
    uint8_t* image_audio_buffer_;
    int image_audio_buffer_time_;
    FrameBuffer* image_frame_buffer_;
};

}  // namespace trinity

#endif  // TRINITY_VIDEO_EXPORT_H
