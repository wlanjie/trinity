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

#include "packet_pool.h"
#include "tools.h"
#include "android_xlog.h"

namespace trinity {

PacketPool::PacketPool()
    : total_discard_video_packet_duration_(0),
      buffer_size_(0),
      buffer_(nullptr),
      buffer_cursor_(0),
      video_packet_duration_(0) ,
      accompany_buffer_size_(0),
      accompany_buffer_(nullptr),
      accompany_buffer_cursor_(0),
      total_discard_video_packet_duration_copy_(0),
      audio_sample_rate_(0),
      channels_(0),
      audio_packet_queue_(nullptr),
      video_packet_queue_(nullptr),
      decoder_packet_queue_(nullptr),
      accompany_packet_queue_(nullptr) {
    pthread_rwlock_init(&rw_lock_, nullptr);
    pthread_rwlock_init(&accompany_drop_frame_lock_, nullptr);
}


PacketPool::~PacketPool() {
    pthread_rwlock_destroy(&rw_lock_);
    pthread_rwlock_destroy(&accompany_drop_frame_lock_);
}

PacketPool* PacketPool::instance_ = new PacketPool();
PacketPool* PacketPool::GetInstance() {
    return instance_;
}

void PacketPool::InitAudioPacketQueue(int audioSampleRate) {
    const char* name = "audioPacket queue_";
    audio_packet_queue_ = new AudioPacketQueue(name);
    this->audio_sample_rate_ = audioSampleRate;
    this->channels_ = INPUT_CHANNEL_4_ANDROID;
    buffer_size_ = audioSampleRate * channels_ * AUDIO_PACKET_DURATION_IN_SECS;
    buffer_ = new short[buffer_size_];
    buffer_cursor_ = 0;
}

void PacketPool::AbortAudioPacketQueue() {
    if (nullptr != audio_packet_queue_) {
        audio_packet_queue_->Abort();
    }
}

void PacketPool::DestroyAudioPacketQueue() {
    if (nullptr != audio_packet_queue_) {
        delete audio_packet_queue_;
        audio_packet_queue_ = nullptr;
    }
    if (nullptr != buffer_) {
        delete[] buffer_;
        buffer_ = nullptr;
    }
}

int PacketPool::GetAudioPacket(AudioPacket **audioPacket, bool block) {
    int result = -1;
    if (nullptr != audio_packet_queue_) {
        result = audio_packet_queue_->Get(audioPacket, block);
    }
    return result;
}

int PacketPool::GetAudioPacketQueueSize() {
    return audio_packet_queue_->Size();
}

bool PacketPool::DiscardAudioPacket() {
    bool ret = false;
    AudioPacket *tempAudioPacket = nullptr;
    int resultCode = audio_packet_queue_->Get(&tempAudioPacket, true);
    if (resultCode > 0) {
        delete tempAudioPacket;
        tempAudioPacket = nullptr;
        pthread_rwlock_wrlock(&rw_lock_);
        total_discard_video_packet_duration_ -= (AUDIO_PACKET_DURATION_IN_SECS * 1000.0f);
        pthread_rwlock_unlock(&rw_lock_);
        ret = true;
    }
    return ret;
}

bool PacketPool::DetectDiscardAudioPacket() {
    bool ret = false;
    pthread_rwlock_wrlock(&rw_lock_);
    ret = total_discard_video_packet_duration_ >= (AUDIO_PACKET_DURATION_IN_SECS * 1000.0f);
    pthread_rwlock_unlock(&rw_lock_);
    return ret;
}

void PacketPool::PushAudioPacketToQueue(AudioPacket *audioPacket) {
    if (nullptr != audio_packet_queue_) {
        int audioPacketBufferCursor = 0;
        while (audioPacket->size > 0) {
            int audioBufferLength = buffer_size_ - buffer_cursor_;
            int length = MIN(audioBufferLength, audioPacket->size);
            memcpy(buffer_ + buffer_cursor_, audioPacket->buffer + audioPacketBufferCursor, length * sizeof(short));
            audioPacket->size -= length;
            buffer_cursor_ += length;
            audioPacketBufferCursor += length;
            if (buffer_cursor_ == buffer_size_) {
                auto* targetAudioPacket = new AudioPacket();
                targetAudioPacket->size = buffer_size_;
                auto * audioBuffer = new short[buffer_size_];
                memcpy(audioBuffer, buffer_, buffer_size_ * sizeof(short));
                targetAudioPacket->buffer = audioBuffer;
                audio_packet_queue_->Put(targetAudioPacket);
                buffer_cursor_ = 0;
            }
        }
        delete audioPacket;
    }
}

void PacketPool::InitDecoderAccompanyPacketQueue() {
    const char *name = "decoder_ accompany packet_ queue_";
    decoder_packet_queue_ = new AudioPacketQueue(name);
}

void PacketPool::AbortDecoderAccompanyPacketQueue() {
    if (nullptr != decoder_packet_queue_) {
        decoder_packet_queue_->Abort();
    }
}

void PacketPool::DestroyDecoderAccompanyPacketQueue() {
    if (nullptr != decoder_packet_queue_) {
        delete decoder_packet_queue_;
        decoder_packet_queue_ = nullptr;
    }
}

int PacketPool::GetDecoderAccompanyPacket(AudioPacket **audioPacket, bool block) {
    int result = -1;
    if (nullptr != decoder_packet_queue_) {
        result = decoder_packet_queue_->Get(audioPacket, block);
    }
    return result;
}

int PacketPool::GeDecoderAccompanyPacketQueueSize() {
    return decoder_packet_queue_->Size();
}

void PacketPool::ClearDecoderAccompanyPacketToQueue() {
    decoder_packet_queue_->Flush();
}

void PacketPool::PushDecoderAccompanyPacketToQueue(AudioPacket *audioPacket) {
    decoder_packet_queue_->Put(audioPacket);
}

void PacketPool::InitAccompanyPacketQueue(int sampleRate, int channels) {
    const char *name = "accompanyPacket queue_";
    accompany_packet_queue_ = new AudioPacketQueue(name);
    /** 初始化 Accompany 缓冲 Buffer **/
    accompany_buffer_size_ = static_cast<int>(sampleRate * channels *
                                              AUDIO_PACKET_DURATION_IN_SECS);
    accompany_buffer_ = new short[accompany_buffer_size_];
    accompany_buffer_cursor_ = 0;
    total_discard_video_packet_duration_copy_ = 0;
}

void PacketPool::AbortAccompanyPacketQueue() {
    if (nullptr != accompany_packet_queue_) {
        accompany_packet_queue_->Abort();
    }
}

void PacketPool::DestoryAccompanyPacketQueue() {
    if (nullptr != accompany_packet_queue_) {
        delete accompany_packet_queue_;
        accompany_packet_queue_ = NULL;
    }
    if (nullptr != accompany_buffer_) {
        delete[] accompany_buffer_;
        accompany_buffer_ = nullptr;
    }
}

int PacketPool::GetAccompanyPacket(AudioPacket **accompanyPacket, bool block) {
    return nullptr == accompany_packet_queue_ ? 0 : accompany_packet_queue_->Get(accompanyPacket, block);
}

int PacketPool::GetAccompanyPacketQueueSize() {
    return accompany_packet_queue_ == nullptr ? 0 : accompany_packet_queue_->Size();
}

void PacketPool::PushAccompanyPacketToQueue(AudioPacket *accompanyPacket) {
    if (nullptr == accompany_packet_queue_) {
        return;
    }
    int audioPacketBufferCursor = 0;
    while (accompanyPacket->size > 0) {
        int audioBufferLength = accompany_buffer_size_ - accompany_buffer_cursor_;
        int length = MIN(audioBufferLength, accompanyPacket->size);
        memcpy(accompany_buffer_ + accompany_buffer_cursor_,
               accompanyPacket->buffer + audioPacketBufferCursor, length * sizeof(short));
        accompanyPacket->size -= length;
        accompany_buffer_cursor_ += length;
        audioPacketBufferCursor += length;
        if (accompany_buffer_cursor_ == accompany_buffer_size_) {
            auto *targetAudioPacket = new AudioPacket();
            targetAudioPacket->size = accompany_buffer_size_;
            auto *audioBuffer = new short[accompany_buffer_size_];
            memcpy(audioBuffer, accompany_buffer_, accompany_buffer_size_ * sizeof(short));
            targetAudioPacket->buffer = audioBuffer;
            targetAudioPacket->position = accompanyPacket->position;
            targetAudioPacket->frameNum = accompanyPacket->frameNum;
            accompany_packet_queue_->Put(targetAudioPacket);
            accompany_buffer_cursor_ = 0;
        }
    }
    delete accompanyPacket;
}

bool PacketPool::DiscardAccompanyPacket() {
    if (nullptr == accompany_packet_queue_) {
        return false;
    }
    bool ret = false;
    AudioPacket *tempAccompanyPacket = NULL;
    int resultCode = accompany_packet_queue_->Get(&tempAccompanyPacket, true);
    if (resultCode > 0) {
        delete tempAccompanyPacket;
        tempAccompanyPacket = NULL;
        pthread_rwlock_wrlock(&accompany_drop_frame_lock_);
        total_discard_video_packet_duration_copy_ -= (AUDIO_PACKET_DURATION_IN_SECS * 1000.0f);
        pthread_rwlock_unlock(&accompany_drop_frame_lock_);
        ret = true;
    }
    return ret;
}

bool PacketPool::DetectDiscardAccompanyPacket() {
    bool ret = false;
    pthread_rwlock_wrlock(&accompany_drop_frame_lock_);
    ret = total_discard_video_packet_duration_copy_ >= (AUDIO_PACKET_DURATION_IN_SECS * 1000.0f);
    pthread_rwlock_unlock(&accompany_drop_frame_lock_);
    return ret;
}

void PacketPool::InitRecordingVideoPacketQueue() {
    if (nullptr == video_packet_queue_) {
        const char* name = "recording video yuv frame_ packet_ queue_";
        video_packet_queue_ = new VideoPacketQueue(name);
        total_discard_video_packet_duration_ = 0;
    }
}

void PacketPool::AbortRecordingVideoPacketQueue() {
    if (nullptr != video_packet_queue_) {
        video_packet_queue_->Abort();
    }
}

void PacketPool::DestroyRecordingVideoPacketQueue() {
    if (nullptr != video_packet_queue_) {
        delete video_packet_queue_;
        video_packet_queue_ = nullptr;
    }
}

int PacketPool::GetRecordingVideoPacket(VideoPacket **videoPacket, bool block, bool wait) {
    int result = -1;
    if (nullptr != video_packet_queue_) {
        result = video_packet_queue_->Get(videoPacket, block);
    }
    return result;
}

bool PacketPool::DetectDiscardVideoPacket() {
    return video_packet_queue_->Size() > VIDEO_PACKET_QUEUE_THRRESHOLD;
}

bool PacketPool::PushRecordingVideoPacketToQueue(VideoPacket *videoPacket) {
    bool dropFrame = false;
    if (nullptr != video_packet_queue_) {
        while (DetectDiscardVideoPacket()) {
            dropFrame = true;
            int discardVideoFrameCnt = 0;
            int discardVideoFrameDuration = video_packet_queue_->DiscardGOP(&discardVideoFrameCnt);
            if (discardVideoFrameDuration < 0) {
                break;
            }
            RecordDropVideoFrame(discardVideoFrameDuration);
        }
        auto duration = video_packet_duration_ - videoPacket->timeMills;
        // 第一帧时,不能计算时长,就按25帧的帧率计算
        videoPacket->duration = duration < 0 ? 40 : duration;
        video_packet_queue_->Put(videoPacket);
        video_packet_duration_ = videoPacket->timeMills;
    }
    return dropFrame;
}

void PacketPool::RecordDropVideoFrame(int discardVideoPacketDuration) {
    pthread_rwlock_wrlock(&rw_lock_);
    total_discard_video_packet_duration_+=discardVideoPacketDuration;
    pthread_rwlock_unlock(&rw_lock_);

    pthread_rwlock_wrlock(&accompany_drop_frame_lock_);
    total_discard_video_packet_duration_copy_ += discardVideoPacketDuration;
    pthread_rwlock_unlock(&accompany_drop_frame_lock_);
}

int PacketPool::GetRecordingVideoPacketQueueSize() {
    if (nullptr != video_packet_queue_) {
        return video_packet_queue_->Size();
    }
    return 0;
}

void PacketPool::ClearRecordingVideoPacketToQueue() {
    if (NULL != video_packet_queue_) {
        return video_packet_queue_->Flush();
    }
}

}  // namespace trinity
