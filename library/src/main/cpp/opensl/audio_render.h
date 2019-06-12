//
// Created by wlanjie on 2019/4/21.
//

#ifndef TRINITY_AUDIO_RENDER_H
#define TRINITY_AUDIO_RENDER_H

#include <fstream>
#include <iostream>
#include "opensl_context.h"
#define DEFAULT_AUDIO_BUFFER_DURATION_IN_SECS 0.03
#define PLAYING_STATE_STOPPED (0x00000001)
#define PLAYING_STATE_PLAYING (0x00000002)

namespace trinity {

typedef int(*AudioPlayerCallback)(uint8_t* , size_t, void* ctx);

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

private:
    //初始化OpenSL要播放的buffer
    void InitPlayerBuffer();
    //释放OpenSL播放的buffer
    void FreePlayerBuffer();
    //实例化一个对象
    SLresult RealizeObject(SLObjectItf object);
    //销毁这个对象以及这个对象下面的接口
    void DestroyObject(SLObjectItf& object);
    //创建输出对象
    SLresult CreateOutputMix();
    //创建OpenSL的AudioPlayer对象
    SLresult CreateAudioPlayer(int channels, int accompanySampleRate);
    //获得AudioPlayer对象的bufferQueue的接口
    SLresult GetAudioPlayerBufferQueueInterface();
    //获得AudioPlayer对象的play的接口
    SLresult GetAudioPlayerPlayInterface();
    //设置播放器的状态为播放状态
    SLresult SetAudioPlayerStatePlaying();
    //设置播放器的状态为暂停状态
    SLresult SetAudioPlayerStatePaused();
    //以下三个是进行注册回调函数的
    //当OpenSL播放完毕给他的buffer数据之后，就会回调playerCallback
    SLresult RegisterPlayerCallback();
    //playerCallback中其实会调用我们的实例方法producePacket
    static void PlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    //这里面会调用在初始化的时候注册过来的回调函数，回调它，由这个回调函数填充数据（这个回调函数还负责音画同步）
    void ProducePacket();

private:
    std::ofstream output_stream_;
    SLEngineItf engine_;
    SLObjectItf output_mix_object_;
    SLObjectItf audio_player_object_;
    SLAndroidSimpleBufferQueueItf audio_player_buffer_queue_;
    SLPlayItf audio_player_play_;

    uint8_t* buffer_;
    size_t buffer_size_;
    int playing_state_;
    AudioPlayerCallback audio_player_callback_;
    void* context_;
};

}

#endif //TRINITY_AUDIO_RENDER_H
