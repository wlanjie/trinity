//
// Created by wlanjie on 2019/4/20.
//

#ifndef TRINITY_RESAMPLE_H
#define TRINITY_RESAMPLE_H

#include <stdint.h>

extern "C" {
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
};

namespace trinity {

class Resample {
public:
    Resample();
    virtual ~Resample();

    virtual int Init(int src_rate = 48000, int dst_rate = 44100, int max_src_nb_samples = 1024, int src_channels = 1, int dst_channels = 1);
    virtual int Process(short* in, uint8_t* out, int src_nb_samples, int* out_nb_samples);
    virtual int Process(short** in, uint8_t* out, int src_nb_samples, int* out_nb_samples);
    virtual int Convert(uint8_t* out, int src_nb_samples, int* out_nb_samples);
    virtual void Destroy();

private:
    int src_rate_;
    int dst_rate_;
    int src_nb_samples_;
    int64_t src_ch_layout_;
    int64_t dst_ch_layout_;
    int src_nb_channels_;
    int dst_nb_channels_;
    int src_line_size_;
    int dst_line_size_;
    int dst_nb_samples_;
    int max_dst_nb_samples_;
    uint8_t** dst_data_;
    uint8_t** src_data_;
    enum AVSampleFormat dst_sample_fmt_;
    int dst_buffer_size_;
    const char* fmt_;
    struct SwrContext* swr_context_;
};

}

#endif //TRINITY_RESAMPLE_H
