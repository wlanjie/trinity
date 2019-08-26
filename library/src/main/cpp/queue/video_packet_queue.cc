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

#include "video_packet_queue.h"
#include "android_xlog.h"

namespace trinity {

VideoPacketQueue::VideoPacketQueue() {
    Init();
}

VideoPacketQueue::VideoPacketQueue(const char *queueNameParam) {
    Init();
    queue_name_ = queueNameParam;
}

void VideoPacketQueue::Init() {
    pthread_mutex_init(&lock_, NULL);
    pthread_cond_init(&condition_, NULL);
    packet_size_ = 0;
    first_ = NULL;
    last_ = NULL;
    abort_request_ = false;
    current_time_mills_ = NON_DROP_FRAME_FLAG;
}

VideoPacketQueue::~VideoPacketQueue() {
    Flush();
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&condition_);
}

int VideoPacketQueue::Size() {
    pthread_mutex_lock(&lock_);
    int size = packet_size_;
    pthread_mutex_unlock(&lock_);
    return size;
}

void VideoPacketQueue::Flush() {
    VideoPacketList *pkt, *pkt1;
    VideoPacket *videoPacket;
    pthread_mutex_lock(&lock_);
    for (pkt = first_; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        videoPacket = pkt->pkt;
        if (NULL != videoPacket) {
            delete videoPacket;
        }
        delete pkt;
        pkt = NULL;
    }
    last_ = NULL;
    first_ = NULL;
    packet_size_ = 0;
    pthread_mutex_unlock(&lock_);
}

int VideoPacketQueue::Put(VideoPacket *pkt) {
    if (abort_request_) {
        delete pkt;
        return -1;
    }
    VideoPacketList *pkt1 = new VideoPacketList();
    if (!pkt1)
        return -1;
    pkt1->pkt = pkt;
    pkt1->next = NULL;
    pthread_mutex_lock(&lock_);
    if (last_ == NULL) {
        first_ = pkt1;
    } else {
        last_->next = pkt1;
    }
    last_ = pkt1;
    packet_size_++;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    return 0;
}

int VideoPacketQueue::DiscardGOP(int *discardVideoFrameCnt) {
    int discardVideoFrameDuration = 0;
    (*discardVideoFrameCnt) = 0;
    VideoPacketList *pktList = 0;
    pthread_mutex_lock(&lock_);
    bool isFirstFrameIDR = false;
    if (first_) {
        VideoPacket *pkt = first_->pkt;
        if (pkt) {
            int nalu_type = pkt->getNALUType();
            if (nalu_type == H264_NALU_TYPE_IDR_PICTURE) {
                isFirstFrameIDR = true;
            }
        }
    }
    for (;;) {
        if (abort_request_) {
            discardVideoFrameDuration = 0;
            break;
        }
        pktList = first_;
        if (pktList) {
            VideoPacket *pkt = pktList->pkt;
            if (pkt) {
                int nalu_type = pkt->getNALUType();
                if (NON_DROP_FRAME_FLAG == current_time_mills_) {
                    current_time_mills_ = pkt->timeMills;
                }
                if (nalu_type == H264_NALU_TYPE_IDR_PICTURE) {
                    if (isFirstFrameIDR) {
                        isFirstFrameIDR = false;
                        first_ = pktList->next;
                        if (!first_) {
                            last_ = NULL;
                        }
                        discardVideoFrameDuration += pkt->duration;
                        (*discardVideoFrameCnt)++;
                        packet_size_--;
                        delete pkt;
                        pkt = NULL;
                        delete pktList;
                        pktList = NULL;
                        continue;
                    } else {
                        break;
                    }
                } else if (nalu_type == H264_NALU_TYPE_NON_IDR_PICTURE) {
                    first_ = pktList->next;
                    if (!first_) {
                        last_ = NULL;
                    }
                    discardVideoFrameDuration += pkt->duration;
                    (*discardVideoFrameCnt)++;
                    packet_size_--;
                    delete pkt;
                    pkt = NULL;
                    delete pktList;
                    pktList = NULL;
                } else {
                    // sps pps 的问题
                    discardVideoFrameDuration = -1;
                    break;
                }
            }
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&lock_);
    LOGI("discardVideoFrameDuration is %d", discardVideoFrameDuration);
    return discardVideoFrameDuration;
}

/* return < 0 if aborted, 0 if no packet_ and > 0 if packet_.  */
int VideoPacketQueue::Get(VideoPacket **pkt, bool block) {
    VideoPacketList *pkt1;
    int ret;
    int getLockCode = pthread_mutex_lock(&lock_);
    for (;;) {
        if (abort_request_) {
            ret = -1;
            break;
        }
        pkt1 = first_;
        if (pkt1) {
            first_ = pkt1->next;
            if (!first_)
                last_ = NULL;
            packet_size_--;
            *pkt = pkt1->pkt;
            if (NON_DROP_FRAME_FLAG != current_time_mills_) {
                (*pkt)->timeMills = current_time_mills_;
                current_time_mills_ += (*pkt)->duration;
            }
            delete pkt1;
            pkt1 = NULL;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&condition_, &lock_);
        }
    }
    pthread_mutex_unlock(&lock_);
    return ret;
}

void VideoPacketQueue::Abort() {
    pthread_mutex_lock(&lock_);
    abort_request_ = true;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
}

}  // namespace trinity
