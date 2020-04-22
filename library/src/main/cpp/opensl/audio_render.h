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

#ifndef TRINITY_AUDIO_RENDER_H
#define TRINITY_AUDIO_RENDER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define DEFAULT_AUDIO_BUFFER_DURATION_IN_SECS 0.03
#define PLAYING_STATE_STOPPED (0x00000001)
#define PLAYING_STATE_PLAYING (0x00000002)

namespace trinity {

typedef int(*AudioPlayerCallback)(uint8_t**, int*, void* context);

class AudioRender {
 public:
    AudioRender();
    virtual ~AudioRender();

    SLresult Init(int channels, int accompanySampleRate, AudioPlayerCallback, void* ctx);
    SLresult Start();
    SLresult Play();
    SLresult Pause();
    SLresult Stop();
    bool IsPlaying();
    long GetCurrentTimeMills();
    void DestroyContext();
    int64_t GetDeltaTime();

 private:
    // 初始化OpenSL要播放的buffer
    void InitPlayerBuffer();
    // 释放OpenSL播放的buffer
    void FreePlayerBuffer();
    // 实例化一个对象
    SLresult RealizeObject(SLObjectItf object);
    // 销毁这个对象以及这个对象下面的接口
    void DestroyObject(SLObjectItf& object);
    // 创建输出对象
    SLresult CreateOutputMix();
    // 创建OpenSL的AudioPlayer对象
    SLresult CreateAudioPlayer(int channels, int accompanySampleRate);
    // 获得AudioPlayer对象的bufferQueue的接口
    SLresult GetAudioPlayerBufferQueueInterface();
    // 获得AudioPlayer对象的play的接口
    SLresult GetAudioPlayerPlayInterface();
    // 设置播放器的状态为播放状态
    SLresult SetAudioPlayerStatePlaying();
    // 设置播放器的状态为暂停状态
    SLresult SetAudioPlayerStatePaused();
    // 以下三个是进行注册回调函数的
    // 当OpenSL播放完毕给他的buffer数据之后，就会回调playerCallback
    SLresult RegisterPlayerCallback();
    // playerCallback中其实会调用我们的实例方法producePacket
    static void PlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    // 这里面会调用在初始化的时候注册过来的回调函数，回调它，由这个回调函数填充数据（这个回调函数还负责音画同步）
    void ProducePacket();

 private:
    SLEngineItf engine_;
    SLObjectItf engine_object_;
    SLObjectItf output_mix_object_;
    SLObjectItf audio_player_object_;
    SLAndroidSimpleBufferQueueItf audio_player_buffer_queue_;
    SLPlayItf audio_player_play_;
    int playing_state_;
    AudioPlayerCallback audio_player_callback_;
    void* context_;
    SLmillisecond play_position_;
};

}  // namespace trinity

#endif  // TRINITY_AUDIO_RENDER_H
