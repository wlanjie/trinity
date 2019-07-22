//
// Created by wlanjie on 2019/4/20.
//

#ifndef TRINITY_AUDIO_ENCODER_H
#define TRINITY_AUDIO_ENCODER_H

#include <stdint.h>
#include "audio_packet_queue.h"

extern "C" {
#include "libavcodec/avcodec.h"
};

namespace trinity {

class AudioEncoder {
private:
    AVCodecContext* codec_context_;
    AVFrame* encode_frame_;
    int64_t audio_next_pts_;
    uint8_t** audio_samples_data_;
    int audio_nb_frames_;
    int audio_samples_size_;
    int bit_rate_;
    int channels_;
    int sample_rate_;

    typedef int (*PCMFrameCallback)(int16_t *, int, int, double *, void *context);
    PCMFrameCallback pcm_frame_callback_;
    void *pcm_frame_context_;
private:
    int AllocFrame();
    int AllocAudioStream(const char* codec_name);

public:
    AudioEncoder();
    virtual ~AudioEncoder();

    int Init(int bit_rate, int channels, int sample_rate, const char* codec_name,
             int (*PCMFrameCallback)(int16_t *, int, int, double *, void *context),
             void* context);

    int Encode(AudioPacket** packet);
    void Destroy();
};

}

#endif //TRINITY_AUDIO_ENCODER_H
