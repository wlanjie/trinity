#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "livecore/common/audio_packet_queue.h"

#ifndef UINT64_C
#define UINT64_C(value)__CONCAT(value,ULL)
#endif

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/samplefmt.h"
#include "libavutil/common.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
};

#ifndef PUBLISH_BITE_RATE
#define PUBLISH_BITE_RATE 64000
#endif

class AudioEncoder {
private:
    /** 音频流数据输出 **/
    AVCodecContext *codec_context_;
    AVFrame *encode_frame_;
    int64_t audio_next_pts_;
    uint8_t **audio_samples_data_;
    int audio_nb_samples_;
    int audio_samples_size_;
    int bit_rate_;
    int channels_;
    int sample_rate_;
    int AllocAVFrame();
    int AllocAudioStream(const char *codec_name);
    typedef int (*FillPCMFrameCallback)(int16_t *, int, int, double *, void *context);
    FillPCMFrameCallback fill_pcm_frame_callback_;
    void *fill_pcm_frame_context_;
public:
    AudioEncoder();
    virtual ~AudioEncoder();
    int Init(int bitRate, int channels, int sampleRate, const char *codec_name,
             int (*FillPCMFrameCallback)(int16_t *, int, int, double *, void *context),
             void *context);
    int Encode(LiveAudioPacket **audioPacket);
    void Destroy();
};

#endif //AUDIO_ENCODER_H
