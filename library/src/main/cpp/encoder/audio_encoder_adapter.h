#ifndef AUDIO_ENCODER_ADAPTER_H
#define AUDIO_ENCODER_ADAPTER_H

#include <pthread.h>
#include "audio_encoder.h"
#include "livecore/common/packet_pool.h"
#include "livecore/common/audio_packet_pool.h"

/**
 * 1:负责组织PCM的Queue
 * 2:负责处理PCM
 * 3:负责打上时间戳输出到AAC的Queue
 *
 */
class AudioEncoderAdapter {
public:
    AudioEncoderAdapter();

    virtual ~AudioEncoderAdapter();

    void Init(LivePacketPool *pcmPacketPool, int audioSampleRate, int audioChannels,
              int audioBitRate, const char *audio_codec_name);

    virtual void Destroy();

protected:
    /** 控制编码线程的状态量 **/
    bool encoding_;
    /** 编码线程的变量与方法 **/
    AudioEncoder *audio_encoder_;
    pthread_t audio_encoder_thread_;
    static void *StartEncodeThread(void *ptr);
    void StartEncode();
    /** 负责从pcmPacketPool中取数据, 调用编码器编码之后放入aacPacketPool中 **/
    LivePacketPool *pcm_packet_pool_;
    LiveAudioPacketPool *aac_packet_pool_;

    /** 由于音频的buffer大小和每一帧的大小不一样，所以我们利用缓存数据的方式来 分次得到正确的音频数据 **/
    int packet_buffer_size_;
    short *packet_buffer_;
    int packet_buffer_cursor_;
    int audio_sample_rate_;
    int audio_channels_;
    int audio_bit_rate_;
    char *audio_codec_name_;
    double packet_buffer_presentation_time_mills_;
    /** 安卓和iOS平台的输入声道是不一样的, 所以设置channelRatio这个参数 **/
    float channel_ratio_;
    int CpyToSamples(int16_t *samples, int samplesInShortCursor, int cpyPacketBufferSize,
                     double *presentationTimeMills);
    int GetAudioPacket();
    virtual int ProcessAudio() {
        return packet_buffer_size_;
    };
    virtual void DiscardAudioPacket();
public:
    int GetAudioFrame(int16_t *samples, int frame_size, int nb_channels, double *presentationTimeMills);

};

#endif // AUDIO_ENCODER_ADAPTER_H
