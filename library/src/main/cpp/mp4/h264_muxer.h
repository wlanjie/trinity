//
// Created by wlanjie on 2019/4/18.
//

#ifndef TRINITY_H264_MUXER_H
#define TRINITY_H264_MUXER_H

#include "mp4_muxer.h"

namespace trinity {

class H264Muxer : public Mp4Muxer {
public:
    H264Muxer();
    virtual ~H264Muxer();
    virtual int Stop();

protected:
    int last_presentation_time_ms_;
    virtual int WriteVideoFrame(AVFormatContext* oc, AVStream* st);
    virtual double GetVideoStreamTimeInSecs();
    uint32_t FindStartCode(uint8_t* in_buffer, uint32_t in_ui32_buffer_size,
            uint32_t in_ui32_code, uint32_t &out_ui32_processed_bytes);
    void ParseH264SequenceHeader(uint8_t* in_buffer, uint32_t in_ui32_size,
            uint8_t** in_sps_buffer, int& in_sps_size,
            uint8_t** in_pps_buffer, int& in_pps_size);
};

}

#endif //TRINITY_H264_MUXER_H
