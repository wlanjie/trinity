#ifndef RECORDING_H264_PUBLISHER_H
#define RECORDING_H264_PUBLISHER_H

#include "mp4_mux.h"

class MuxH264: public Mp4Mux {
public:
    MuxH264();
    virtual ~MuxH264();
    
public:
    virtual int Stop();
    
protected:
    int lastPresentationTimeMs;
    
    /** 4、为视频流写入一帧数据 **/
    virtual int WriteVideoFrame(AVFormatContext *oc, AVStream *st);
    virtual double GetVideoStreamTimeInSecs();
    
    uint32_t FindStartCode(uint8_t *in_pBuffer, uint32_t in_ui32BufferSize,
                           uint32_t in_ui32Code, uint32_t &out_ui32ProcessedBytes);
    
    void ParseH264SequenceHeader(uint8_t *in_pBuffer, uint32_t in_ui32Size,
                                 uint8_t **inout_pBufferSPS, int &inout_ui32SizeSPS,
                                 uint8_t **inout_pBufferPPS, int &inout_ui32SizePPS);
};

#endif //RECORDING_H264_PUBLISHER_H
