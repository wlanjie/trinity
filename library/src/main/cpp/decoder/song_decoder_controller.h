#ifndef LIVE_SONG_DECODER_CONTROLLER_H
#define LIVE_SONG_DECODER_CONTROLLER_H

#include <unistd.h>
#include "libresampler/resampler.h"
#include "accompany_decoder.h"
#include "aac_accompany_decoder.h"

#define CHANNEL_PER_FRAME    2
#define BITS_PER_CHANNEL     16
#define BITS_PER_BYTE        8
/** decode data to queue and queue size **/
#define QUEUE_SIZE_MAX_THRESHOLD    45
#define QUEUE_SIZE_MIN_THRESHOLD    30

#define ACCOMPANY_TYPE_SILENT_SAMPLE          0
#define ACCOMPANY_TYPE_SONG_SAMPLE            1

/**
 * 这是伴奏单独的控制类，原唱的控制类继承自这个类
 */
class LiveSongDecoderController {
protected:
    /** 本身这个类也是一个生产者，每次客户端读出一个packet去我们就会把伴奏的packet给放入队列中 **/
    LiveCommonPacketPool *packet_pool_;
    /** 伴奏的解码器 **/
    AccompanyDecoder *accompany_decoder_;
    /** 是否需要将伴奏重采样成为人声的采样频率 **/
    bool need_resample_;
    Resampler *resampler_;
    /** 伴奏和原唱的采样频率与解码伴奏和原唱的每个packet的大小 **/
    int accompany_sample_rate_;
    short *silent_samples_;
    int accompany_packet_buffer_size_;
    int vocal_sample_rate_;
    long frameNum;

    /** 由于可能要进行音量的变化，这个是控制音量的值 **/
    float volume_;
    float accompany_max_;

    /** 解码线程，主要是调用原唱和伴唱的解码器 解码出对应的packet数据来 **/
    pthread_t song_decoder_thread_;

    virtual int InitAccompanyDecoder(const char *accompanyPath);
    static void *StartDecoderThread(void *ptr);

    /** 开启解码线程 **/
    virtual void InitDecoderThread();

    virtual void DecodeSongPacket();

    /** 挂起解码线程 **/
    void SuspendDecoderThread();

    /** 让解码线程继续运行 **/
    void ResumeDecoderThread();

    /** 销毁解码线程 **/
    virtual void DestroyDecoderThread();

    void DestroyResampler();

    void DestroyAccompanyDecoder();

    int buffer_queue_size_;
    int buffer_queue_cursor_;
    short *buffer_queue_;

    /** 将accompanyPacket放入到accompanyQueue中 **/
    void PushAccompanyPacketToQueue(LiveAudioPacket *accompanyPacket);
    int BuildSlientSamples(short *samples);

public:
    /** 由于在解码线程中要用到以下几个值，所以访问控制符是public的 **/
    bool running_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    int accompany_type_;
    bool suspend_flag_;
    pthread_mutex_t suspend_lock_;
    pthread_cond_t suspend_condition_;
    LiveSongDecoderController();
    ~LiveSongDecoderController();

    /** 初始两个decoder,并且根据上一步算出的采样率，计算出伴奏和原唱的bufferSize **/
    virtual void Init(float packetBufferTimePercent, int vocalSampleRate);

    /** 读取samples,返回实际读取的sample的个数 并且将accompanyPacket放入到accompanyQueue中**/
    virtual int ReadSamplesAndProducePacket(short *samples, int size, int *slientSizeArr, int *extraAccompanyTypeArr);

    /** 设置伴奏或者原唱的音量 **/
    void SetVolume(float volumeParam, float accompanyMaxParam) {
        volume_ = volumeParam;
        accompany_max_ = accompanyMaxParam;
    }

    /** Mini播放器的一系列操作 **/
    void StartAccompany(const char *accompanyPath);

    void PauseAccompany();

    void ResumeAccompany();

    void StopAccompany();

    /** 销毁这个controller **/
    virtual void Destroy();
};

#endif //LIVE_SONG_DECODER_CONTROLLER_H
