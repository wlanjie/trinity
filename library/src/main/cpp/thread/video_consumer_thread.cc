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
 *
 */

//
// Created by wlanjie on 2019/4/17.
//

#include "video_consumer_thread.h"
#include "android_xlog.h"

namespace trinity {

VideoConsumerThread::VideoConsumerThread()
    : running_(false),
      video_packet_pool_(nullptr),
      audio_packet_pool_(nullptr),
      stopping_(false),
      mp4_muxer_(nullptr) {
    pthread_mutex_init(&lock_, nullptr);
    pthread_cond_init(&condition_, nullptr);
}

VideoConsumerThread::~VideoConsumerThread() {}

static int AudioPacketCallback(AudioPacket** packet, void* context) {
    VideoConsumerThread* thread = reinterpret_cast<VideoConsumerThread*>(context);
    return thread->GetAudioPacket(packet);
}

static int VideoPacketCallback(VideoPacket** packet, void* context) {
    VideoConsumerThread* thread = reinterpret_cast<VideoConsumerThread*>(context);
    return thread->GetH264Packet(packet);
}

int VideoConsumerThread::GetH264Packet(VideoPacket** packet) {
    return video_packet_pool_->GetRecordingVideoPacket(packet, true, false);
}

int VideoConsumerThread::GetAudioPacket(AudioPacket** packet) {
    return audio_packet_pool_->GetAudioPacket(packet, true, false);
}

int VideoConsumerThread::Init(const char* path, int video_width, int video_height, int frame_rate, int video_bit_Rate,
         int audio_sample_rate, int audio_channels, int audio_bit_rate, std::string& audio_codec_name) {
    Init();
    if (nullptr == mp4_muxer_) {
        mp4_muxer_ = new Mp4Muxer();
        int ret = mp4_muxer_->Init(path, video_width, video_height, frame_rate, video_bit_Rate, audio_sample_rate, audio_channels, audio_bit_rate, audio_codec_name);
        if (ret < 0) {
            Release();
            return ret;
        }
        if (!stopping_) {
            mp4_muxer_->RegisterAudioPacketCallback(AudioPacketCallback, this);
            mp4_muxer_->RegisterVideoPacketCallback(VideoPacketCallback, this);
        }
    }
    return 0;
}

void VideoConsumerThread::Start() {
    HandleRun(nullptr);
}

void VideoConsumerThread::StartAsync() {
    LOGI("enter: %s", __func__);
    stopping_ = false;
    pthread_create(&thread_, nullptr, StartThread, this);
    LOGI("leave: %s", __func__);
}

int VideoConsumerThread::Wait() {
    if (!running_) {
        return 0;
    }
    return pthread_join(thread_, nullptr);
}

void VideoConsumerThread::WaitOnNotify() {
    pthread_mutex_lock(&lock_);
    pthread_cond_wait(&condition_, &lock_);
    pthread_mutex_unlock(&lock_);
}

void VideoConsumerThread::Notify() {
    pthread_mutex_lock(&lock_);
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
}

void VideoConsumerThread::Stop() {
//    if (!running_) {
//        return;
//    }
    if (nullptr == video_packet_pool_) {
        return;
    }
    if (nullptr == audio_packet_pool_) {
        return;
    }
    LOGI("enter: %s", __func__);
    stopping_ = true;
    video_packet_pool_->AbortRecordingVideoPacketQueue();
    audio_packet_pool_->AbortAudioPacketQueue();
    pthread_join(thread_, nullptr);
    Release();
    video_packet_pool_->DestroyRecordingVideoPacketQueue();
    audio_packet_pool_->DestroyAudioPacketQueue();
    LOGI("leave: %s", __func__);
}

void VideoConsumerThread::HandleRun(void *context) {
    LOGI("enter: %s", __func__);
    while (!stopping_) {
        int ret = mp4_muxer_->Encode();
        if (ret < 0) {
            break;
        }
    }
    LOGI("leave: %s", __func__);
}

void *VideoConsumerThread::StartThread(void *context) {
    LOGI("enter: %s", __func__);
    VideoConsumerThread* thread = static_cast<VideoConsumerThread *>(context);
    thread->running_ = true;
    thread->HandleRun(context);
    thread->running_ = false;
    pthread_exit(0);
}

void VideoConsumerThread::Init() {
    stopping_ = false;
    video_packet_pool_ = PacketPool::GetInstance();
    audio_packet_pool_ = AudioPacketPool::GetInstance();
}

void VideoConsumerThread::Release() {
    if (nullptr != mp4_muxer_) {
        mp4_muxer_->Stop();
        delete mp4_muxer_;
        mp4_muxer_ = nullptr;
    }
}

}  // namespace trinity
