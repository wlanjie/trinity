#ifndef SONG_STUDIO_H264_UTIL_H
#define SONG_STUDIO_H264_UTIL_H

#include "CommonTools.h"
#include <vector>

using namespace std;

#define LOG_TAG "SONG_STUDIO_H264_UTIL"

#define _H264_NALU_TYPE_NON_IDR_PICTURE                                  	1
#define _H264_NALU_TYPE_IDR_PICTURE                                      	5
#define _H264_NALU_TYPE_SEQUENCE_PARAMETER_SET                           	7
#define _H264_NALU_TYPE_PICTURE_PARAMETER_SET								8
#define _H264_NALU_TYPE_SEI          	                              	    6

#define is_h264_start_code(code)	(((code) & 0x0ffffff) == 0x01)
#define is_h264_start_code_prefix(code)	(code == 0x00)

typedef struct NALUnit{
	int 				naluType;
	uint8_t* 		naluBody;
	int 				naluSize;
	NALUnit(int type, uint8_t* body){
		this->naluType = type;
		this->naluBody = body;
	}
} NALUnit;

static inline uint32_t findAnnexbStartCode(uint8_t* in_pBuffer, uint32_t in_ui32BufferSize,
			uint32_t in_ui32Code, uint32_t& out_ui32ProcessedBytes) {
	uint32_t ui32Code = in_ui32Code;
	const uint8_t * ptr = in_pBuffer;
	while (ptr < in_pBuffer + in_ui32BufferSize) {
		ui32Code = *ptr++ + (ui32Code << 8);
		if (is_h264_start_code(ui32Code)){
			//如果判断到 0x01
			uint32_t firstPrefix = *(ptr - 3);
			uint32_t secondPrefix = *(ptr - 2);
			if(is_h264_start_code_prefix(firstPrefix) &&
				is_h264_start_code_prefix(secondPrefix)){
				//并且判断到他的前面三个都是0x00 就说明这是一个startCode
				break;
			}
		}
	}
	out_ui32ProcessedBytes = (uint32_t)(ptr - in_pBuffer);
	return ui32Code;
}

static inline void parseH264SpsPps(uint8_t* in_pBuffer, uint32_t in_ui32Size,
		vector<NALUnit*>* units) {
//	LOGI("enter parseH264SpsPps... in_ui32Size is %d", in_ui32Size);
	uint32_t ui32StartCode = 0x0ff;

	uint8_t* pBuffer = in_pBuffer;
	uint32_t ui32BufferSize = in_ui32Size;

	NALUnit* tempUnit = NULL;
	uint8_t* tempBufferCursor = pBuffer;
	uint32_t ui32ProcessedBytes = 0;
	do {
		ui32StartCode = findAnnexbStartCode(pBuffer, ui32BufferSize, ui32StartCode,
				ui32ProcessedBytes);
		pBuffer += ui32ProcessedBytes;
		ui32BufferSize -= ui32ProcessedBytes;

		if (ui32BufferSize < 1)
			break;

		uint8_t nalu_type = (*pBuffer & 0x1f);
		switch(nalu_type){
			case _H264_NALU_TYPE_SEQUENCE_PARAMETER_SET:
				tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
				tempUnit = new NALUnit(_H264_NALU_TYPE_SEQUENCE_PARAMETER_SET, tempBufferCursor);
			break;
			case _H264_NALU_TYPE_PICTURE_PARAMETER_SET:
				tempUnit->naluSize = ui32ProcessedBytes - 3;
				units->push_back(tempUnit);
				tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
				tempUnit = new NALUnit(_H264_NALU_TYPE_PICTURE_PARAMETER_SET, tempBufferCursor);
			break;
			case _H264_NALU_TYPE_SEI:
                if (tempUnit != NULL){
                    tempUnit->naluSize = ui32ProcessedBytes - 3;
                    units->push_back(tempUnit);
                }
				tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
				tempUnit = new NALUnit(_H264_NALU_TYPE_SEI, tempBufferCursor);
			break;
			case _H264_NALU_TYPE_IDR_PICTURE:
                if (tempUnit != NULL) {
                    tempUnit->naluSize = ui32ProcessedBytes - 3;
                    units->push_back(tempUnit);
                }
				tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
				tempUnit = new NALUnit(_H264_NALU_TYPE_IDR_PICTURE, tempBufferCursor);
			break;
			case _H264_NALU_TYPE_NON_IDR_PICTURE:
				if(tempUnit != NULL){
					tempUnit->naluSize = ui32ProcessedBytes - 3;
					units->push_back(tempUnit);
				}
				tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
				tempUnit = new NALUnit(_H264_NALU_TYPE_NON_IDR_PICTURE, tempBufferCursor);
			break;
		}
	} while (ui32BufferSize > 0);
	tempUnit->naluSize = ui32ProcessedBytes;
	units->push_back(tempUnit);
}

#endif //SONG_STUDIO_H264_UTIL_H
