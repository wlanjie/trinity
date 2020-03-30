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
// Created by wlanjie on 2019/4/19.
//

#include "record_processor.h"
#include "tools.h"
#include "android_xlog.h"

#define AUDIO_CHANNEL 1

#define MIN_DIFF_TIME_MILLS 50
#define MAX_DIFF_TIME_MILLS 150

namespace trinity {

RecordProcessor::RecordProcessor() {
    audio_sample_rate_ = 0;
    audio_samples_ = nullptr;
    audio_sample_time_mills_ = 0;
    audio_sample_cursor_ = 0;
    audio_buffer_size_ = 0;
    audio_buffer_time_mills_ = 0;
    packet_pool_ = nullptr;
    recording_flag_ = false;
    start_time_mills_ = 0;
    data_accumulate_time_mills_ = 0;
    audio_encoder_ = nullptr;
}

RecordProcessor::~RecordProcessor() {}

void RecordProcessor::InitAudioBufferSize(int sample_rate, int audio_buffer_size, float speed) {
    LOGI("%s sample_rate: %d audio_buffer_size: %d", __FUNCTION__, sample_rate, audio_buffer_size);
    audio_sample_cursor_ = 0;
    audio_buffer_size_ = audio_buffer_size;
    audio_sample_rate_ = sample_rate;
    audio_samples_ = new short[audio_buffer_size];
    packet_pool_ = PacketPool::GetInstance();
    audio_buffer_time_mills_ = static_cast<int>(audio_buffer_size * 1000.0f / audio_sample_rate_);
    sonic_stream_ = sonicCreateStream(sample_rate, AUDIO_CHANNEL);
    sonicSetSpeed(sonic_stream_, speed);
}

int RecordProcessor::PushAudioBufferToQueue(short *samples, int size) {
    if (size <= 0) {
        return size;
    }
    if (!recording_flag_) {
        recording_flag_ = true;
        start_time_mills_ = currentTimeMills();
    }
    sonicWriteShortToStream(sonic_stream_, samples, size);
    int sample_written = 0;
    auto* out_buffer = new short[2048];
    do {
        sample_written = sonicReadShortFromStream(sonic_stream_, out_buffer, 2048);
        int samples_cursor = 0;
        int samples_count = sample_written;
        while (samples_count > 0) {
            if ((audio_sample_cursor_ + samples_count) < audio_buffer_size_) {
                this->CopyToAudioSamples(out_buffer + samples_cursor, samples_count);
                audio_sample_cursor_ += samples_count;
                samples_cursor += samples_count;
                samples_count = 0;
            } else {
                int subFullSize = audio_buffer_size_ - audio_sample_cursor_;
                this->CopyToAudioSamples(out_buffer + samples_cursor, subFullSize);
                audio_sample_cursor_ += subFullSize;
                samples_cursor += subFullSize;
                samples_count -= subFullSize;
                FlushAudioBufferToQueue();
            }
        }
    } while (sample_written > 0);
    delete[] out_buffer;
    return size;
}

void RecordProcessor::FlushAudioBufferToQueue() {
    if (audio_sample_cursor_ > 0) {
        if (NULL == audio_encoder_) {
            audio_encoder_ = new AudioEncoderAdapter();
            int audioBitRate = 128 * 1024;
            const char* audioCodecName = "libfdk_aac";
            audio_encoder_->Init(packet_pool_, audio_sample_rate_, AUDIO_CHANNEL, audioBitRate, audioCodecName);
        }
        short* packetBuffer = new short[audio_sample_cursor_];
        if (NULL == packetBuffer) {
            return;
        }
        memcpy(packetBuffer, audio_samples_, audio_sample_cursor_ * sizeof(short));
        AudioPacket * audioPacket = new AudioPacket();
        audioPacket->buffer = packetBuffer;
        audioPacket->size = audio_sample_cursor_;
        packet_pool_->PushAudioPacketToQueue(audioPacket);
        audio_sample_cursor_ = 0;
        data_accumulate_time_mills_+=audio_buffer_time_mills_;
        int correctDurationInTimeMills = 0;
        if (DetectNeedCorrect(data_accumulate_time_mills_, audio_sample_time_mills_, &correctDurationInTimeMills)) {
            // 检测到有问题了, 需要进行修复
            this->CorrectRecordBuffer(correctDurationInTimeMills);
        }
    }
}

void RecordProcessor::Destroy() {
    LOGI("enter %s", __FUNCTION__);
    if (nullptr != audio_samples_) {
        delete[] audio_samples_;
        audio_samples_ = nullptr;
    }
    if (nullptr != audio_encoder_) {
        audio_encoder_->Destroy();
        delete audio_encoder_;
        audio_encoder_ = nullptr;
    }
    sonicDestroyStream(sonic_stream_);
    LOGI("leave %s", __FUNCTION__);
}

bool RecordProcessor::DetectNeedCorrect(int64_t data_present_time_mills,
                int64_t recording_time_mills, int *correct_time_mills) {
    bool ret = false;
    (*correct_time_mills) = 0;
    if (data_present_time_mills <= (recording_time_mills - MAX_DIFF_TIME_MILLS)) {
        ret = true;
        (*correct_time_mills) = MAX_DIFF_TIME_MILLS - MIN_DIFF_TIME_MILLS;
    }
    return ret;
}

void RecordProcessor::CopyToAudioSamples(short *buffer, int length) {
    if (0 == audio_sample_cursor_) {
        audio_sample_time_mills_ = currentTimeMills() - start_time_mills_;
    }
    memcpy(audio_samples_ + audio_sample_cursor_, buffer, length * sizeof(short));
}

int RecordProcessor::CorrectRecordBuffer(int correct_time_mills) {
    int correctBufferSize = static_cast<int>((correct_time_mills / 1000.0f) * audio_sample_rate_);
    AudioPacket * audioPacket = GetSilentDataPacket(correctBufferSize);
    packet_pool_->PushAudioPacketToQueue(audioPacket);
    // 混音
//    AudioPacket * accompanyPacket = GetSilentDataPacket(correctBufferSize);
//    packet_pool_->pushAccompanyPacketToQueue(accompanyPacket);
//    data_accumulate_time_mills_ += correct_time_mills;
    return 0;
}

AudioPacket* RecordProcessor::GetSilentDataPacket(int audioBufferSize) {
    AudioPacket * audioPacket = new AudioPacket();
    audioPacket->buffer = new short[audioBufferSize];
    memset(audioPacket->buffer, 0, audioBufferSize * sizeof(short));
    audioPacket->size = audioBufferSize;
    return audioPacket;
}

}  // namespace trinity
