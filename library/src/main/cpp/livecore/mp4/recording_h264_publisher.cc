#include "./recording_h264_publisher.h"
#define LOG_TAG "RecordingH264Publisher"
#define is_start_code(code)	(((code) & 0x0ffffff) == 0x01)

RecordingH264Publisher::RecordingH264Publisher() {
    headerData = NULL;
    headerSize = 0;
    lastPresentationTimeMs = -1;
}

RecordingH264Publisher::~RecordingH264Publisher() {
}

double RecordingH264Publisher::getVideoStreamTimeInSecs(){
    return lastPresentationTimeMs / 1000.0f;
}

uint32_t RecordingH264Publisher::findStartCode(uint8_t* in_pBuffer, uint32_t in_ui32BufferSize,
                                               uint32_t in_ui32Code, uint32_t& out_ui32ProcessedBytes) {
    uint32_t ui32Code = in_ui32Code;
    
    const uint8_t * ptr = in_pBuffer;
    while (ptr < in_pBuffer + in_ui32BufferSize) {
        ui32Code = *ptr++ + (ui32Code << 8);
        if (is_start_code(ui32Code))
            break;
    }
    
    out_ui32ProcessedBytes = (uint32_t)(ptr - in_pBuffer);
    
    return ui32Code;
}

void RecordingH264Publisher::parseH264SequenceHeader(uint8_t* in_pBuffer, uint32_t in_ui32Size,
                                                     uint8_t** inout_pBufferSPS, int& inout_ui32SizeSPS,
                                                     uint8_t** inout_pBufferPPS, int& inout_ui32SizePPS) {
    uint32_t ui32StartCode = 0x0ff;
    
    uint8_t* pBuffer = in_pBuffer;
    uint32_t ui32BufferSize = in_ui32Size;
    
    uint32_t sps = 0;
    uint32_t pps = 0;
    
    uint32_t idr = in_ui32Size;
    
    do {
        uint32_t ui32ProcessedBytes = 0;
        ui32StartCode = findStartCode(pBuffer, ui32BufferSize, ui32StartCode,
                                      ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;
        
        if (ui32BufferSize < 1)
            break;
        
        uint8_t val = (*pBuffer & 0x1f);
        
        if (val == 5)
            idr = pps + ui32ProcessedBytes - 4;
        
        if (val == 7)
            sps = ui32ProcessedBytes;
        
        if (val == 8)
            pps = sps + ui32ProcessedBytes;
        
    } while (ui32BufferSize > 0);
    
    *inout_pBufferSPS = in_pBuffer + sps - 4;
    inout_ui32SizeSPS = pps - sps;
    
    *inout_pBufferPPS = in_pBuffer + pps - 4;
    inout_ui32SizePPS = idr - pps + 4;
}

int RecordingH264Publisher::write_video_frame(AVFormatContext *oc, AVStream *st) {
    int ret = 0;
    AVCodecContext *c = st->codec;
    
    /** 1、调用注册的回调方法来拿到我们的h264的EncodedData **/
    LiveVideoPacket *h264Packet = NULL;
    fillH264PacketCallback(&h264Packet, fillH264PacketContext);
    if (h264Packet == NULL) {
        LOGE("fillH264PacketCallback get null packet");
        return VIDEO_QUEUE_ABORT_ERR_CODE;
    }
    int bufferSize = (h264Packet)->size;
    uint8_t* outputData = (uint8_t *) ((h264Packet)->buffer);
    lastPresentationTimeMs = h264Packet->timeMills;
    /** 2、填充起来我们的AVPacket **/
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    pkt.stream_index = st->index;
//    int64_t pts = lastPresentationTimeMs / 1000.0f / av_q2d(video_st->time_base);
    int64_t cal_pts = lastPresentationTimeMs / 1000.0f / av_q2d(video_st->time_base);
    int64_t pts = h264Packet->pts == PTS_PARAM_UN_SETTIED_FLAG ? cal_pts : h264Packet->pts;
    int64_t dts = h264Packet->dts == DTS_PARAM_UN_SETTIED_FLAG ? pts : h264Packet->dts == DTS_PARAM_NOT_A_NUM_FLAG ? AV_NOPTS_VALUE : h264Packet->dts;
//    LOGI("h264Packet is {%llu, %llu}", h264Packet->pts, h264Packet->dts);
    int nalu_type = (outputData[4] & 0x1F);
//    LOGI("Final is {%llu, %llu} nalu_type is %d", pts, dts, nalu_type);
    if (nalu_type == H264_NALU_TYPE_SEQUENCE_PARAMETER_SET) {
        //我们这里要求sps和pps一块拼接起来构造成AVPacket传过来
        headerSize = bufferSize;
        headerData = new uint8_t[headerSize];
        memcpy(headerData, outputData, bufferSize);
        
        uint8_t* spsFrame = 0;
        uint8_t* ppsFrame = 0;
        
        int spsFrameLen = 0;
        int ppsFrameLen = 0;
        
        parseH264SequenceHeader(headerData, headerSize, &spsFrame, spsFrameLen,
                                &ppsFrame, ppsFrameLen);
        
        // Extradata contains PPS & SPS for AVCC format
        int extradata_len = 8 + spsFrameLen - 4 + 1 + 2 + ppsFrameLen - 4;
        c->extradata = (uint8_t*) av_mallocz(extradata_len);
        c->extradata_size = extradata_len;
        c->extradata[0] = 0x01;
        c->extradata[1] = spsFrame[4 + 1];
        c->extradata[2] = spsFrame[4 + 2];
        c->extradata[3] = spsFrame[4 + 3];
        c->extradata[4] = 0xFC | 3;
        c->extradata[5] = 0xE0 | 1;
        int tmp = spsFrameLen - 4;
        c->extradata[6] = (tmp >> 8) & 0x00ff;
        c->extradata[7] = tmp & 0x00ff;
        int i = 0;
        for (i = 0; i < tmp; i++)
            c->extradata[8 + i] = spsFrame[4 + i];
        c->extradata[8 + tmp] = 0x01;
        int tmp2 = ppsFrameLen - 4;
        c->extradata[8 + tmp + 1] = (tmp2 >> 8) & 0x00ff;
        c->extradata[8 + tmp + 2] = tmp2 & 0x00ff;
        for (i = 0; i < tmp2; i++)
            c->extradata[8 + tmp + 3 + i] = ppsFrame[4 + i];
        
        int ret = avformat_write_header(oc, NULL);
        if (ret < 0) {
            LOGI("Error occurred when opening output file: %s\n", av_err2str(ret));
        } else{
        		isWriteHeaderSuccess = true;
        }
    } else {
        if (nalu_type == H264_NALU_TYPE_IDR_PICTURE || nalu_type == H264_NALU_TYPE_SEI) {
            pkt.size = bufferSize;
            pkt.data = outputData;
            
           if(pkt.data[0] == 0x00 && pkt.data[1] == 0x00 &&
					pkt.data[2] == 0x00 && pkt.data[3] == 0x01){
				bufferSize -= 4;
				pkt.data[0] = ((bufferSize) >> 24) & 0x00ff;
				pkt.data[1] = ((bufferSize) >> 16) & 0x00ff;
				pkt.data[2] = ((bufferSize) >> 8) & 0x00ff;
				pkt.data[3] = ((bufferSize)) & 0x00ff;
			}
			
            pkt.pts = pts;
            pkt.dts = dts;
            pkt.flags = AV_PKT_FLAG_KEY;
            c->frame_number++;
        }
        else {
            pkt.size = bufferSize;
            pkt.data = outputData;
            
			if(pkt.data[0] == 0x00 && pkt.data[1] == 0x00 &&
					pkt.data[2] == 0x00 && pkt.data[3] == 0x01){
				bufferSize -= 4;
				pkt.data[0] = ((bufferSize ) >> 24) & 0x00ff;
				pkt.data[1] = ((bufferSize ) >> 16) & 0x00ff;
				pkt.data[2] = ((bufferSize ) >> 8) & 0x00ff;
				pkt.data[3] = ((bufferSize )) & 0x00ff;
			}
			
            pkt.pts = pts;
            pkt.dts = dts;
            pkt.flags = 0;
            c->frame_number++;
        }
        /** 3、写出数据 **/
        if (pkt.size) {
//        		LOGI("pkt : {%llu, %llu}", pkt.pts, pkt.dts);
            ret = RecordingPublisher::interleavedWriteFrame(oc, &pkt);
            if (ret != 0) {
                LOGI("Error while writing Video frame: %s\n", av_err2str(ret));
            }
//            av_free_packet(&pkt);
        } else {
            ret = 0;
        }
    }
    delete h264Packet;
    return ret;
}

int RecordingH264Publisher::stop() {
    int ret = RecordingPublisher::stop();
    if (headerData) {
        delete [] headerData;
        headerData = NULL;
    }
    return ret;
}
