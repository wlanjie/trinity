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
 *
 */

//
// Created by wlanjie on 2019/4/17.
//

#ifndef TRINITY_VIDEO_PACKET_QUEUE_H
#define TRINITY_VIDEO_PACKET_QUEUE_H

#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define H264_NALU_TYPE_NON_IDR_PICTURE                                  1
#define H264_NALU_TYPE_IDR_PICTURE                                      5
#define H264_NALU_TYPE_SEQUENCE_PARAMETER_SET                           7
#define H264_NALU_TYPE_PICTURE_PARAMETER_SET							8
#define H264_NALU_TYPE_SEI                                          	6

#define NON_DROP_FRAME_FLAG                                             -1.0f
#define DTS_PARAM_UN_SETTIED_FLAG										-1
#define DTS_PARAM_NOT_A_NUM_FLAG										-2
#define PTS_PARAM_UN_SETTIED_FLAG										-1

namespace trinity {

typedef struct VideoPacket {
    uint8_t* buffer;
    int size;
    int timeMills;
    int duration;
    int64_t pts;
    int64_t dts;

    VideoPacket() {
        buffer = nullptr;
        size = 0;
        pts = PTS_PARAM_UN_SETTIED_FLAG;
        dts = DTS_PARAM_UN_SETTIED_FLAG;
        duration = 0;
        timeMills = 0;
    }
    ~VideoPacket() {
        if (nullptr != buffer) {
            delete[] buffer;
            buffer = nullptr;
        }
    }
    int getNALUType() {
        int nalu_type = H264_NALU_TYPE_NON_IDR_PICTURE;
        if (nullptr != buffer) {
            nalu_type = (buffer[4] & 0x1F);
        }
        return nalu_type;
    }
    bool isIDRFrame() {
        bool ret = false;
        if (getNALUType() == H264_NALU_TYPE_IDR_PICTURE) {
            ret = true;
        }
        return ret;
    }
    VideoPacket* clone() {
        VideoPacket* result = new VideoPacket();
        result->buffer = new uint8_t[size];
        memcpy(result->buffer, buffer, size);
        result->size = size;
        result->timeMills = timeMills;
        return result;
    }
} VideoPacket;

typedef struct VideoPacketList {
    VideoPacket *pkt;
    struct VideoPacketList *next;
    VideoPacketList() {
        pkt = nullptr;
        next = nullptr;
    }
} VideoPacketList;

class VideoPacketQueue {
 public:
    VideoPacketQueue();
    explicit VideoPacketQueue(const char* queueNameParam);
    ~VideoPacketQueue();

    void Init();
    void Flush();
    int Put(VideoPacket *videoPacket);
    /* return < 0 if aborted, 0 if no packet_ and > 0 if packet_.  */
    int Get(VideoPacket **videoPacket, bool block);
    int DiscardGOP(int *discardVideoFrameCnt);
    int Size();
    void Abort();

 private:
    VideoPacketList* first_;
    VideoPacketList* last_;
    int packet_size_;
    bool abort_request_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    const char* queue_name_;
    float current_time_mills_;
};

}  // namespace trinity

#endif  // TRINITY_VIDEO_PACKET_QUEUE_H
