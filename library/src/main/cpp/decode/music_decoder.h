//
// Created by wlanjie on 2019/4/20.
//

#ifndef TRINITY_MUSIC_DECODER_H
#define TRINITY_MUSIC_DECODER_H

#include <fstream>
#include <iostream>
#include "packet_pool.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
};

namespace trinity {

class MusicDecoder {
public:
    MusicDecoder();
    ~MusicDecoder();

    virtual int Init(const char* path, int packet_buffer_size);
    virtual int Init(const char* path);
    virtual void SetPacketBufferSize(int packet_buffer_size);
    virtual AudioPacket* DecodePacket();
    virtual void SeekFrame();
    virtual void Destroy();
    virtual int GetSampleRate();
    void SetSeekReq(bool seek_req);
    bool HasSeekReq();
    bool HasSeekResp();
    /** 设置到播放到什么位置，单位是秒，但是后边3位小数，其实是精确到毫秒 **/
    void SetPosition(float seconds);
    float GetActualSeekPosition();

private:
    int ReadSamples(short* samples, int size);
    int ReadFrame();
    bool AudioCodecIsSupported();
private:
    bool seek_req_;
    bool seek_resp_;
    float seek_seconds_;
    float actual_seek_position_;

    AVFormatContext* format_context_;
    AVCodecContext* codec_context_;
    int stream_index_;
    float time_base_;
    AVFrame* audio_frame_;
    AVPacket packet_;
    char* path_;
    bool seek_success_read_frame_success_;
    int packet_buffer_size_;

    short* audio_buffer_;
    float position_;
    int audio_buffer_cursor_;
    int audio_buffer_size_;
    float duration_;
    bool need_first_frame_correct_flag_;
    float first_frame_correction_in_secs_;
    SwrContext* swr_context_;
    void* swr_buffer_;
    int swr_buffer_size_;

    std::ofstream output_stream_;
};

}

#endif //TRINITY_MUSIC_DECODER_H
