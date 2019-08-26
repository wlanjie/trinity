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

namespace trinity {

AudioRender::AudioRender()
    : engine_(nullptr),
      output_mix_object_(nullptr),
      audio_player_object_(nullptr),
      audio_player_buffer_queue_(nullptr),
      audio_player_play_(nullptr),
      buffer_(nullptr),
      buffer_size_(0),
      playing_state_(-1),
      audio_player_callback_(nullptr),
      context_(nullptr) {
}

AudioRender::~AudioRender() {}

SLresult AudioRender::RegisterPlayerCallback() {
    // Register the player callback
    return (*audio_player_buffer_queue_)->RegisterCallback(audio_player_buffer_queue_, PlayerCallback, this);
}

void AudioRender::PlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioRender* audioRender = reinterpret_cast<AudioRender*>(context);
    audioRender->ProducePacket();
}
void AudioRender::ProducePacket() {
    // 回调playerController中的方法来获得buffer
    if (playing_state_ == PLAYING_STATE_PLAYING) {
        int actualSize = audio_player_callback_(buffer_, buffer_size_, context_);
        if (actualSize > 0 && playing_state_ == PLAYING_STATE_PLAYING) {
            // 将提供的数据加入到播放的buffer中去
            (*audio_player_buffer_queue_)->Enqueue(audio_player_buffer_queue_, buffer_, actualSize);
        }
    }
}

SLresult AudioRender::Pause() {
    return SetAudioPlayerStatePaused();
}

SLresult AudioRender::Stop() {
    // Set the audio player state playing
    SLresult result = SetAudioPlayerStatePaused();
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    playing_state_ = PLAYING_STATE_STOPPED;
    usleep(0.05 * 1000000);
    DestroyContext();
    return SL_RESULT_SUCCESS;
}

SLresult AudioRender::Play() {
    // Set the audio player state playing
    SLresult result = SetAudioPlayerStatePlaying();
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    playing_state_ = PLAYING_STATE_PLAYING;
    return SL_RESULT_SUCCESS;
}

SLresult AudioRender::Start() {
    // Set the audio player state playing
    SLresult result = SetAudioPlayerStatePlaying();
    if (SL_RESULT_SUCCESS != result) {
        return result;
    }
    playing_state_ = PLAYING_STATE_PLAYING;
    ProducePacket();
    return SL_RESULT_SUCCESS;
}

SLresult AudioRender::Init(int channels, int accompanySampleRate, AudioPlayerCallback produceDataCallback, void* ctx) {
    context_ = ctx;
    audio_player_callback_ = produceDataCallback;
    SLresult result = SL_RESULT_UNKNOWN_ERROR;
    OpenSLContext* openSLESContext = OpenSLContext::GetInstance();
    engine_ = openSLESContext->GetEngine();

    if (engine_ == nullptr){
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

    // Calculate the buffer size default 50ms length
    buffer_size_ = 2048;
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
    LOGI("leave DestroyContext");
}

/** 以下是私有方法的实现 **/
SLresult AudioRender::RealizeObject(SLObjectItf object) {
    return (*object)->Realize(object, SL_BOOLEAN_FALSE); // No async, blocking call
}

void AudioRender::DestroyObject(SLObjectItf& object) {
    if (0 != object)
        (*object)->Destroy(object);
    object = 0;
}

SLresult AudioRender::CreateOutputMix() {
    // Create output mix object
    return (*engine_)->CreateOutputMix(engine_, &output_mix_object_, 0, // no interfaces
                                            0, // no interfaces
                                            0); // no required
}

void AudioRender::InitPlayerBuffer() {
    // Initialize buffer
    buffer_ = new uint8_t[2048];
}

void AudioRender::FreePlayerBuffer() {
    if (nullptr != buffer_) {
        delete[] buffer_;
        buffer_ = NULL;
    }
}

SLresult AudioRender::CreateAudioPlayer(int channels, int accompanySampleRate) {
    SLDataLocator_AndroidSimpleBufferQueue dataSourceLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                                 1
    };
    int samplesPerSec = OpenSLSampleRate(accompanySampleRate);
    int channelMask = GetChannelMask(channels);
    SLDataFormat_PCM dataSourceFormat = { SL_DATAFORMAT_PCM, // format type
                                          (SLuint32)channels, // channel count
                                          (SLuint32)samplesPerSec, // samples per second in millihertz
                                          SL_PCMSAMPLEFORMAT_FIXED_16, // bits per sample
                                          SL_PCMSAMPLEFORMAT_FIXED_16, // container size
                                          (SLuint32)channelMask, // channel mask
                                          SL_BYTEORDER_LITTLEENDIAN // endianness
    };

    // Data source is a simple buffer queue with PCM format
    SLDataSource dataSource = { &dataSourceLocator, // data locator
                                &dataSourceFormat // data format
    };

    // Output mix locator for data sink
    SLDataLocator_OutputMix dataSinkLocator = { SL_DATALOCATOR_OUTPUTMIX, // locator type
                                                output_mix_object_ // output mix
    };

    // Data sink is an output mix
    SLDataSink dataSink = { &dataSinkLocator, // locator
                            0 // format
    };

    // Interfaces that are requested
    SLInterfaceID interfaceIds[] = { SL_IID_BUFFERQUEUE };

    // Required interfaces. If the required interfaces
    // are not available the request will fail
    SLboolean requiredInterfaces[] = { SL_BOOLEAN_TRUE // for SL_IID_BUFFERQUEUE
    };

    // Create audio player object
    return (*engine_)->CreateAudioPlayer(engine_, &audio_player_object_, &dataSource, &dataSink, ARRAY_LEN(interfaceIds), interfaceIds, requiredInterfaces);
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
