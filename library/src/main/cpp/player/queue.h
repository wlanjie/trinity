//
//  queue.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef queue_h
#define queue_h

#include "play_types.h"

FramePool* frame_pool_create(int size);
void frame_pool_release(FramePool *pool);
AVFrame* frame_pool_get_frame(FramePool *pool);
void frame_pool_unref_frame(FramePool *pool, AVFrame *frame);

FrameQueue* frame_queue_create(unsigned int size);
void frame_queue_free(FrameQueue *queue);
int frame_queue_put(FrameQueue *queue, AVFrame *frame);
AVFrame* frame_queue_get(FrameQueue *queue);
void frame_queue_flush(FrameQueue *queue, FramePool *pool);

PacketPool* packet_pool_create(int size);
void packet_pool_reset(PacketPool *pool);
void packet_pool_release(PacketPool *pool);
AVPacket* packet_pool_get_packet(PacketPool *pool);
void packet_pool_unref_packet(PacketPool *pool, AVPacket *packet);

PacketQueue* queue_create(unsigned int size);
void queue_set_duration(PacketQueue *queue, uint64_t max_duration);
void packet_queue_free(PacketQueue *queue);
int packet_queue_put(PacketQueue *queue, AVPacket *packet);
AVPacket* packet_queue_get(PacketQueue *queue);
void packet_queue_flush(PacketQueue *queue, PacketPool *pool);

#endif // queue_h
