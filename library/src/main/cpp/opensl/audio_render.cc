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
// Created by wlanjie on 2019/4/21.
//

#include <unistd.h>
#include "audio_render.h"
#include "android_xlog.h"
#include "tools.h"
#include "opensl_util.h"

namespace trinity {

AudioRender::AudioRender()
      : engine_(nullptr)
      , output_mix_object_(nullptr)
      , audio_player_object_(nullptr)
      , audio_player_buffer_queue_(nullptr)
      , audio_player_play_(nullptr)
      , playing_state_(-1)
      , audio_player_callback_(nullptr)
      , context_(nullptr)
      , play_position_(0) {
}

AudioRender::~AudioRender() {}

SLresult AudioRender::RegisterPlayerCallback() {
    // Register the player callback
    return (*audio_player_buffer_queue_)->RegisterCallback(audio_player_buffer_queue_, PlayerCallback, this);
}

void AudioRender::PlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    auto* audioRender = reinterpret_cast<AudioRender*>(context);
    audioRender->ProducePacket();
}
void AudioRender::ProducePacket() {
    // 回调playerController中的方法来获得buffer
    if (playing_state_ == PLAYING_STATE_PLAYING) {
        uint8_t* buffer = nullptr;
        int buffer_size;
        int ret = audio_player_callback_(&buffer, &buffer_size, context_);
        if (ret == -1) {
            LOGI("ret == -1 set play paused state");
            (*audio_player_play_)->SetPlayState(audio_player_play_, SL_PLAYSTATE_PAUSED);
            return;
        }
        if (ret == 0 && buffer_size > 0 &&
            buffer != nullptr && playing_state_ == PLAYING_STATE_PLAYING) {
            // 将提供的数据加入到播放的buffer中去
            (*audio_player_buffer_queue_)->Enqueue(audio_player_buffer_queue_, buffer,
                                                   static_cast<SLuint32>(buffer_size));
            (*audio_player_play_)->GetPosition(audio_player_play_, &play_position_);
        }
    }
}

int64_t AudioRender::GetDeltaTime() {
    SLmillisecond sec;
    (*audio_player_play_)->GetPosition(audio_player_play_, &sec);
    if (sec > play_position_) {
        return (sec - play_position_) * 1000;
    }
    return 0;
}

SLresult AudioRender::Pause() {
    LOGI("enter: %s", __func__);
    return SetAudioPlayerStatePaused();
}

SLresult AudioRender::Stop() {
    // Set the audio player state playing
    LOGI("enter: %s", __func__);
    SLresult result = SetAudioPlayerStatePaused();
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    playing_state_ = PLAYING_STATE_STOPPED;
    usleep(0.05 * 1000000);
    DestroyContext();
    play_position_ = 0;
    LOGI("leave: %s", __func__);
    return SL_RESULT_SUCCESS;
}

SLresult AudioRender::Play() {
    LOGI("enter: %s", __func__);
    if (nullptr == audio_player_play_) {
        return SL_RESULT_SUCCESS;
    }
    SLuint32 state = SL_PLAYSTATE_PLAYING;
    (*audio_player_play_)->GetPlayState(audio_player_play_, &state);
    if (state != SL_PLAYSTATE_PAUSED) {
        return SL_RESULT_SUCCESS;
    }

    ProducePacket();
    // Set the audio player state playing
    SLresult result = SetAudioPlayerStatePlaying();
    if (SL_RESULT_SUCCESS != result) {
        LOGE("Set Audio play error: %d", result);
        return result;
    }
    playing_state_ = PLAYING_STATE_PLAYING;
    LOGI("leave: %s", __func__);
    return SL_RESULT_SUCCESS;
}

SLresult AudioRender::Start() {
    // Set the audio player state playing
    LOGI("enter %s", __func__);
    SLresult result = SetAudioPlayerStatePlaying();
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    playing_state_ = PLAYING_STATE_PLAYING;
    ProducePacket();
    LOGI("leave %s", __func__);
    return SL_RESULT_SUCCESS;
}

SLresult AudioRender::Init(int channels, int accompanySampleRate, AudioPlayerCallback produceDataCallback, void* ctx) {
    LOGI("enter: %s channels: %d sample_rate: %d", __func__, channels, accompanySampleRate);
    context_ = ctx;
    audio_player_callback_ = produceDataCallback;
    SLresult result = SL_RESULT_UNKNOWN_ERROR;

    // OpenSL ES for Android is designed to be thread-safe,
    // so this option request will be ignored, but it will
    // make the source code portable to other platforms.
    SLEngineOption engineOptions[] = {{(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE}};

    // Create the OpenSL ES engine object
    result = slCreateEngine(&engine_object_, ARRAY_LEN(engineOptions), engineOptions, 0,
                          0,
                          0);
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    result = (*engine_object_)->Realize(engine_object_, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    result = (*engine_object_)->GetInterface(engine_object_, SL_IID_ENGINE, &engine_);
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    if (engine_ == nullptr) {
        return result;
    }

    // Create output mix object
    result = CreateOutputMix();
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }

    // Realize output mix object
    result = RealizeObject(output_mix_object_);
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }

    // Initialize buffer
    InitPlayerBuffer();

    // Create the buffer queue audio player object
    result = CreateAudioPlayer(channels, accompanySampleRate);
    if (SL_RESULT_SUCCESS != result) {
        FreePlayerBuffer();
        DestroyObject(output_mix_object_);
        return result;
    }

    // Realize audio player object
    result = RealizeObject(audio_player_object_);
    if (SL_RESULT_SUCCESS != result) {
        FreePlayerBuffer();
        DestroyObject(output_mix_object_);
        return result;
    }

    // Get audio player buffer queue interface
    result = GetAudioPlayerBufferQueueInterface();
    if (SL_RESULT_SUCCESS != result) {
        DestroyObject(audio_player_object_);
        FreePlayerBuffer();
        DestroyObject(output_mix_object_);
        return result;
    }

    // Registers the player callback
    result = RegisterPlayerCallback();
    if (SL_RESULT_SUCCESS != result) {
        DestroyObject(audio_player_object_);
        FreePlayerBuffer();
        DestroyObject(output_mix_object_);
        return result;
    }

    // Get audio player play interface
    result = GetAudioPlayerPlayInterface();
    if (SL_RESULT_SUCCESS != result) {
        DestroyObject(audio_player_object_);
        FreePlayerBuffer();
        DestroyObject(output_mix_object_);
        return result;
    }
    return SL_RESULT_SUCCESS;
}

long AudioRender::GetCurrentTimeMills() {
    SLmillisecond position = 0;
    if (0 != audio_player_object_ && nullptr != (*audio_player_play_)) {
        SLresult result = (*audio_player_play_)->GetPosition(audio_player_play_, &position);
    }
    return position;
}

bool AudioRender::IsPlaying() {
    bool result = false;
    SLuint32 pState = SL_PLAYSTATE_PLAYING;
    if (0 != audio_player_object_ && nullptr != (*audio_player_play_)) {
        (*audio_player_play_)->GetPlayState(audio_player_play_, &pState);
    } else {
        result = false;
    }
    if (pState == SL_PLAYSTATE_PLAYING) {
        result = true;
    }
    return result;
}

void AudioRender::DestroyContext() {
    LOGI("enter DestroyContext");
    // Destroy audio player object
    DestroyObject(audio_player_object_);
    // Free the player buffer
    FreePlayerBuffer();
    // Destroy output mix object
    DestroyObject(output_mix_object_);
    DestroyObject(engine_object_);
    LOGI("leave DestroyContext");
}

/** 以下是私有方法的实现 **/
SLresult AudioRender::RealizeObject(SLObjectItf object) {
    // No async, blocking call
    return (*object)->Realize(object, SL_BOOLEAN_FALSE);
}

void AudioRender::DestroyObject(SLObjectItf& object) {
    if (nullptr != object) {
        (*object)->Destroy(object);
    }
    object = nullptr;
}

SLresult AudioRender::CreateOutputMix() {
    // Create output mix object
    // no interfaces
    return (*engine_)->CreateOutputMix(engine_, &output_mix_object_, 0,
                                            0,
                                            0);
}

void AudioRender::InitPlayerBuffer() {
    // Initialize buffer
}

void AudioRender::FreePlayerBuffer() {
}

SLresult AudioRender::CreateAudioPlayer(int channels, int accompanySampleRate) {
    SLDataLocator_AndroidSimpleBufferQueue dataSourceLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                                 1
    };
    int samplesPerSec = OpenSLSampleRate(accompanySampleRate);
    int channelMask = GetChannelMask(channels);
    SLDataFormat_PCM dataSourceFormat = { SL_DATAFORMAT_PCM,
                                          (SLuint32)channels,
                                          (SLuint32)samplesPerSec,
                                          SL_PCMSAMPLEFORMAT_FIXED_16,
                                          SL_PCMSAMPLEFORMAT_FIXED_16,
                                          (SLuint32)channelMask,
                                          SL_BYTEORDER_LITTLEENDIAN
    };

    // Data source is a simple buffer queue with PCM format
    SLDataSource dataSource = { &dataSourceLocator,
                                &dataSourceFormat
    };

    // Output mix locator for data sink
    SLDataLocator_OutputMix dataSinkLocator = { SL_DATALOCATOR_OUTPUTMIX,
                                                output_mix_object_
    };

    // Data sink is an output mix
    SLDataSink dataSink = { &dataSinkLocator,
                            0
    };

    // Interfaces that are requested
    SLInterfaceID interfaceIds[] = { SL_IID_BUFFERQUEUE };

    // Required interfaces. If the required interfaces
    // are not available the request will fail
    SLboolean requiredInterfaces[] = { SL_BOOLEAN_TRUE
    };

    // Create audio player object
    return (*engine_)->CreateAudioPlayer(engine_, &audio_player_object_, &dataSource,
                 &dataSink, ARRAY_LEN(interfaceIds), interfaceIds, requiredInterfaces);
}

SLresult AudioRender::GetAudioPlayerBufferQueueInterface() {
    return (*audio_player_object_)->GetInterface(audio_player_object_, SL_IID_BUFFERQUEUE, &audio_player_buffer_queue_);
}

SLresult AudioRender::GetAudioPlayerPlayInterface() {
    return (*audio_player_object_)->GetInterface(audio_player_object_, SL_IID_PLAY, &audio_player_play_);
}

SLresult AudioRender::SetAudioPlayerStatePlaying() {
    LOGI("SetAudioPlayerStatePlaying");
    SLresult result = (*audio_player_play_)->SetPlayState(audio_player_play_, SL_PLAYSTATE_PLAYING);
    return result;
}

SLresult AudioRender::SetAudioPlayerStatePaused() {
    LOGI("SetAudioPlayerStatePaused");
    SLresult result = (*audio_player_play_)->SetPlayState(audio_player_play_, SL_PLAYSTATE_PAUSED);
    return result;
}

}  // namespace trinity
