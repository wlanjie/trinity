/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by wlanjie on 2019/4/17.
//

#ifndef TRINITY_H264_UTIL_H
#define TRINITY_H264_UTIL_H

#include <vector>
#include <stdint.h>

#define _H264_NALU_TYPE_NON_IDR_PICTURE                                  	1 // NOLINT
#define _H264_NALU_TYPE_IDR_PICTURE                                      	5 // NOLINT
#define _H264_NALU_TYPE_SEQUENCE_PARAMETER_SET                           	7 // NOLINT
#define _H264_NALU_TYPE_PICTURE_PARAMETER_SET								8 // NOLINT
#define _H264_NALU_TYPE_SEI          	                              	    6 // NOLINT

#define is_h264_start_code(code) (((code) & 0x0ffffff) == 0x01)
#define is_h264_start_code_prefix(code) (code == 0x00)

typedef struct NALUnit {
    int naluType;
    uint8_t* naluBody;
    int naluSize;

    NALUnit(int type, uint8_t* body) {
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
        if (is_h264_start_code(ui32Code)) {
            // 如果判断到 0x01
            uint32_t firstPrefix = *(ptr - 3);
            uint32_t secondPrefix = *(ptr - 2);
            if (is_h264_start_code_prefix(firstPrefix) &&
               is_h264_start_code_prefix(secondPrefix)) {
                // 并且判断到他的前面三个都是0x00 就说明这是一个startCode
                break;
            }
        }
    }
    out_ui32ProcessedBytes = (uint32_t)(ptr - in_pBuffer);
    return ui32Code;
}

static inline void parseH264SpsPps(uint8_t* in_pBuffer, uint32_t in_ui32Size,
                                   std::vector<NALUnit*>* units) {
    uint32_t ui32StartCode = 0x0ff;

    uint8_t* pBuffer = in_pBuffer;
    uint32_t ui32BufferSize = in_ui32Size;

    NALUnit* tempUnit = nullptr;
    uint8_t* tempBufferCursor = pBuffer;
    uint32_t ui32ProcessedBytes = 0;
    do {
        ui32StartCode = findAnnexbStartCode(pBuffer, ui32BufferSize, ui32StartCode,
                                            ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;

        if (ui32BufferSize < 1) {
            break;
        }

        uint8_t nalu_type = (*pBuffer & 0x1f);
        switch (nalu_type) {
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
                if (tempUnit != nullptr) {
                    tempUnit->naluSize = ui32ProcessedBytes - 3;
                    units->push_back(tempUnit);
                }
                tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
                tempUnit = new NALUnit(_H264_NALU_TYPE_SEI, tempBufferCursor);
                break;
            case _H264_NALU_TYPE_IDR_PICTURE:
                if (tempUnit != nullptr) {
                    tempUnit->naluSize = ui32ProcessedBytes - 3;
                    units->push_back(tempUnit);
                }
                tempBufferCursor = tempBufferCursor + ui32ProcessedBytes;
                tempUnit = new NALUnit(_H264_NALU_TYPE_IDR_PICTURE, tempBufferCursor);
                break;
            case _H264_NALU_TYPE_NON_IDR_PICTURE:
                if (tempUnit != nullptr) {
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


#endif  // TRINITY_H264_UTIL_H
