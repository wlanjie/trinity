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
// Created by wlanjie on 2019/4/20.
//

#ifndef TRINITY_MUSIC_DECODER_CONTROLLER_H
#define TRINITY_MUSIC_DECODER_CONTROLLER_H

#include "packet_pool.h"
#include "resample.h"
#include "music_decoder.h"
#include "audio_render.h"
#include "handler.h"
#include "buffer_pool.h"

#define CHANNEL_PER_FRAME    2
#define BITS_PER_CHANNEL     16
#define BITS_PER_BYTE        8
/** decode data to queue and queue size **/
#define QUEUE_SIZE_MAX_THRESHOLD    45
#define QUEUE_SIZE_MIN_THRESHOLD    30

#define ACCOMPANY_TYPE_SILENT_SAMPLE          0
#define ACCOMPANY_TYPE_SONG_SAMPLE            1

namespace trinity {

class MusicDecoderController : public Handler {
 public:
    MusicDecoderController();
    virtual ~MusicDecoderController();
    virtual void Init(float packet_buffer_time_percent, int vocal_sample_rate);
    virtual int ReadSamplesAndProducePacket(short* samples, int size, int* slient_size, int* extra_accompany_type);
    virtual int ReadSamples(uint8_t* samples, int size);
    void SetVolume(float volume, float volume_max);
    void Start(const char* path);
    void Pause();
    void Resume();
    void Stop();
    void Destroy();

 private:
    static void* MessageQueueThread(void* arg);
    void ProcessMessage();
    void HandleMessage(Message* msg);

    virtual void InitWithMessage(float packet_buffer_time_percent, int vocal_sample_rate);
    virtual void StartWithMessage(char* path);
    virtual void ResumeWithMessage();
    virtual void PauseWithMessage();
    virtual void StopWithMessage();
    virtual void DestroyWithMessage();
    virtual int InitDecoder(const char* path);
    virtual int InitRender();
    static void* StartDecoderThread(void* context);
    virtual void InitDecoderThread();
    virtual void DecodePacket();
    void DestroyResample();
    void DestroyDecoder();
    void DestroyRender();
    void PushPacketToQueue(AudioPacket* packet);
    int BuildSamples(short* samples);
    static int AudioCallback(uint8_t** buffer, int* buffer_size, void* context);

 private:
    pthread_t message_queue_thread_;
    MessageQueue* message_queue_;

    bool running_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    int accompany_type_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    uint8_t* buffer_;
    int buffer_size_;
    PacketPool* packet_pool_;
    MusicDecoder* decoder_;
    Resample* resample_;
    bool need_resample_;
    AudioRender* audio_render_;
    int accompany_sample_rate_;
    short *silent_samples_;
    int accompany_packet_buffer_size_;
    int vocal_sample_rate_;
    float volume_;
    float volume_max_;
    pthread_t decoder_thread_;
    int buffer_queue_size_;
    int buffer_queue_cursor_;
    short* buffer_queue_;
    BufferPool* buffer_pool_;
};

}  // namespace trinity

#endif  // TRINITY_MUSIC_DECODER_CONTROLLER_H
