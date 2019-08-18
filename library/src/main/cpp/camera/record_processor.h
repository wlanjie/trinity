//
// Created by wlanjie on 2019/4/19.
//

#ifndef TRINITY_RECORD_PROCESSOR_H
#define TRINITY_RECORD_PROCESSOR_H

#include <stdint.h>
#include "packet_pool.h"
#include "audio_encoder_adapter.h"

namespace trinity {

class RecordProcessor {
public:
    RecordProcessor();
    ~RecordProcessor();

    void InitAudioBufferSize(int sample_rate, int audio_buffer_size);
    int PushAudioBufferToQueue(short* samples, int size);
    void FlushAudioBufferToQueue();
    void Destroy();

private:
    bool DetectNeedCorrect(int64_t data_present_time_mills, int64_t recording_time_mills, int *correct_time_mills);
    void CopyToAudioSamples(short* buffer, int length);
    int CorrectRecordBuffer(int correct_time_mills);
    AudioPacket* GetSilentDataPacket(int audioBufferSize);
private:
    int audio_sample_rate_;
    short* audio_samples_;
    float audio_sample_time_mills_;
    int audio_sample_cursor_;
    int audio_buffer_size_;
    int audio_buffer_time_mills_;
    PacketPool* packet_pool_;
    bool recording_flag_;
    int64_t start_time_mills_;
    int64_t data_accumulate_time_mills_;
    AudioEncoderAdapter* audio_encoder_;
};

}

#endif //TRINITY_RECORD_PROCESSOR_H
