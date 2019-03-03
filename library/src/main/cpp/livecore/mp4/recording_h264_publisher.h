#ifndef RECORDING_H264_PUBLISHER_H
#define RECORDING_H264_PUBLISHER_H

#include "./recording_publisher.h"

class RecordingH264Publisher: public RecordingPublisher {
public:
    RecordingH264Publisher();
    virtual ~RecordingH264Publisher();
    
public:
    virtual int stop();
    
protected:
    int lastPresentationTimeMs;
    
    /** 4、为视频流写入一帧数据 **/
    virtual int write_video_frame(AVFormatContext *oc, AVStream *st);
    virtual double getVideoStreamTimeInSecs();
    
    uint32_t findStartCode(uint8_t* in_pBuffer, uint32_t in_ui32BufferSize,
                           uint32_t in_ui32Code, uint32_t& out_ui32ProcessedBytes);
    
    void parseH264SequenceHeader(uint8_t* in_pBuffer, uint32_t in_ui32Size,
                                 uint8_t** inout_pBufferSPS, int& inout_ui32SizeSPS,
                                 uint8_t** inout_pBufferPPS, int& inout_ui32SizePPS);
};

#endif //RECORDING_H264_PUBLISHER_H
