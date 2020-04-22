//
//  queue.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "queue.h"
#include <pthread.h>

static void get_frame_defaults(AVFrame *frame) {
    // from ffmpeg libavutil frame.c
    if (frame->extended_data != frame->data) {
        av_freep(&frame->extended_data);
    }
    memset(frame, 0, sizeof(*frame));
    frame->pts                   =
    frame->pkt_dts               = AV_NOPTS_VALUE;
    frame->best_effort_timestamp = AV_NOPTS_VALUE;
    frame->pkt_duration        = 0;
    frame->pkt_pos             = -1;
    frame->pkt_size            = -1;
    frame->key_frame           = 1;
    frame->sample_aspect_ratio = (AVRational){ 0, 1 };
    frame->format              = -1; /* unknown */
    frame->extended_data       = frame->data;
    frame->color_primaries     = AVCOL_PRI_UNSPECIFIED;
    frame->color_trc           = AVCOL_TRC_UNSPECIFIED;
    frame->colorspace          = AVCOL_SPC_UNSPECIFIED;
    frame->color_range         = AVCOL_RANGE_UNSPECIFIED;
    frame->chroma_location     = AVCHROMA_LOC_UNSPECIFIED;
    frame->flags               = 0;
    frame->width               = 0;
    frame->height              = 0;
}

FramePool* frame_pool_create(int size){
    LOGE("frame_pool_create");
    FramePool* pool = (FramePool *)malloc(sizeof(FramePool));
    pool->size = size;
    pool->count = 0;
    pool->index = 0;
    pool->frames = (AVFrame *)av_mallocz(sizeof(AVFrame) * size);
    for(int i = 0; i < size; i++){
        get_frame_defaults(&pool->frames[i]);
    }
    return pool;
}

void frame_pool_release(FramePool *pool) {
    LOGE("frame_pool_release");
    av_free(pool->frames);
    free(pool);
}

AVFrame* frame_pool_get_frame(FramePool *pool){
    AVFrame* p = &pool->frames[pool->index];
    pool->index = (pool->index + 1) % pool->size;
    pool->count++;
    return p;
}

void frame_pool_unref_frame(FramePool *pool, AVFrame *frame){
    av_frame_unref(frame);
    pool->count--;
}

FrameQueue* frame_queue_create(unsigned int size){
    FrameQueue * queue = (FrameQueue *)malloc(sizeof(FrameQueue));
    queue->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    queue->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(queue->mutex, NULL);
    pthread_cond_init(queue->cond, NULL);
    queue->read_index = 0;
    queue->write_index = 0;
    queue->count = 0;
    queue->size = size;
    queue->frames = (AVFrame **)malloc(sizeof(AVFrame *) * size);
    return queue;
}
void frame_queue_free(FrameQueue *queue){
    pthread_mutex_destroy(queue->mutex);
    pthread_cond_destroy(queue->cond);
    free(queue->mutex);
    free(queue->cond);
    free(queue->frames);
    free(queue);
}

int frame_queue_put(FrameQueue *queue, AVFrame *frame){
    pthread_mutex_lock(queue->mutex);
    while(queue->count == queue->size){
        pthread_cond_wait(queue->cond, queue->mutex);
    }
    queue->frames[queue->write_index] = frame;
    queue->write_index = (queue->write_index + 1) % queue->size;
    queue->count++;
    pthread_mutex_unlock(queue->mutex);
    return 0;
}

AVFrame* frame_queue_get(FrameQueue *queue){
    pthread_mutex_lock(queue->mutex);
    if (queue->count == 0) {
        pthread_mutex_unlock(queue->mutex);
        return NULL;
    }
    AVFrame * frame = queue->frames[queue->read_index];
    queue->read_index = (queue->read_index + 1) % queue->size;
    queue->count--;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
    return frame;
}

void frame_queue_flush(FrameQueue *queue, FramePool *pool){
    pthread_mutex_lock(queue->mutex);
    while (queue->count > 0) {
        AVFrame * frame = queue->frames[queue->read_index];
        if (frame != &queue->flush_frame) {
            frame_pool_unref_frame(pool, frame);
        }
        queue->read_index = (queue->read_index + 1) % queue->size;
        queue->count--;
    }
    queue->read_index = 0;
    queue->frames[0] = &queue->flush_frame;
    queue->write_index = 1;
    queue->count = 1;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
}

static void packet_pool_double_size(PacketPool * pool) {
    // step1  malloc new memery space to store pointers |________________|
    AVPacket** temp_packets = (AVPacket **) av_malloc(sizeof(AVPacket *) * pool->size * 2);
    // step2  copy old pointers to new space            |XXXXXXXX________|
    memcpy(temp_packets, pool->packets, sizeof(AVPacket *) * pool->size);
    // step3 fill rest space with av_packet_alloc       |XXXXXXXXOOOOOOOO|
    for (int i = pool->size; i < pool->size * 2; i++) {
        temp_packets[i] = av_packet_alloc();
    }
    // step4 free old pointers space
    free(pool->packets);
    pool->packets = temp_packets;
    // step5 当前指针位置移动到后半部分
    pool->index = pool->size;
    pool->size *= 2;
    LOGI("packet pool double size. new size ==> %d", pool->size);
}

PacketPool* packet_pool_create(int size) {
    PacketPool* pool = (PacketPool *)malloc(sizeof(PacketPool));
    pool->size = size;
    pool->packets = (AVPacket **)av_malloc(sizeof(AVPacket *) * size);
    for (int i = 0; i < pool->size; i++){
        pool->packets[i] = av_packet_alloc();
    }
    return pool;
}

void packet_pool_reset(PacketPool *pool) {
    pool->count = 0;
    pool->index = 0;
}

void packet_pool_release(PacketPool *pool) {
    for (int i = 0; i < pool->size; i++) {
        AVPacket * p = pool->packets[i];
        av_packet_free(&p);
    }
    free(pool->packets);
    free(pool);
}
AVPacket * packet_pool_get_packet(PacketPool *pool) {
    if (pool->count > pool->size / 2) {
        packet_pool_double_size(pool);
    }
    AVPacket* p = pool->packets[pool->index];
    pool->index = (pool->index + 1) % pool->size;
    pool->count ++;
    return p;
}
void packet_pool_unref_packet(PacketPool *pool, AVPacket *packet) {
    av_packet_unref(packet);
    pool->count--;
}

static void packet_double_size(PacketQueue *queue) {
    AVPacket **temp_packets = (AVPacket **) malloc(sizeof(AVPacket *) * queue->size * 2);
    if (queue->write_index == 0) {
        memcpy(temp_packets, queue->packets, sizeof(AVPacket *) * queue->size);
        queue->write_index = queue->size;
    } else {
        memcpy(temp_packets, queue->packets, sizeof(AVPacket *) * queue->write_index);
        memcpy(temp_packets + (queue->write_index + queue->size), queue->packets + queue->write_index,
               sizeof(AVPacket *) * (queue->size - queue->read_index));
        queue->read_index += queue->size;
    }
    free(queue->packets);
    queue->packets = temp_packets;
    queue->size *= 2;
}

PacketQueue* queue_create(unsigned int size) {
    PacketQueue *queue = (PacketQueue *) malloc(sizeof(PacketQueue));
    queue->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    queue->cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(queue->mutex, NULL);
    pthread_cond_init(queue->cond, NULL);
    queue->read_index = 0;
    queue->write_index = 0;
    queue->count = 0;
    queue->size = size;
    queue->duration = 0;
    queue->max_duration = 0;
    queue->total_bytes = 0;
    queue->flush_packet.duration = 0;
    queue->flush_packet.size = 0;
    queue->packets = (AVPacket **) malloc(sizeof(AVPacket *) * size);
    queue->full_cb = NULL;
    queue->empty_cb = NULL;
    return queue;
}

void queue_set_duration(PacketQueue *queue, uint64_t max_duration) {
    queue->max_duration = max_duration;
}

void packet_queue_free(PacketQueue *queue) {
    pthread_mutex_destroy(queue->mutex);
    pthread_cond_destroy(queue->cond);
    free(queue->mutex);
    free(queue->cond);
    free(queue->packets);
    free(queue);
}

int packet_queue_put(PacketQueue *queue, AVPacket *packet) {
    pthread_mutex_lock(queue->mutex);
    if (queue->max_duration > 0 && queue->duration + packet->duration > queue->max_duration) {
        if (queue->full_cb != NULL) {
            queue->full_cb(queue->cb_data);
        }
        pthread_cond_wait(queue->cond, queue->mutex);
    }
    if (queue->count == queue->size) {
        packet_double_size(queue);
    }
    queue->duration += packet->duration;
    queue->packets[queue->write_index] = packet;
    queue->write_index = (queue->write_index + 1) % queue->size;
    queue->count++;
    queue->total_bytes += packet->size;
    pthread_mutex_unlock(queue->mutex);
    return 0;
}

AVPacket *packet_queue_get(PacketQueue *queue) {
    pthread_mutex_lock(queue->mutex);
    if (queue->count == 0) {
        pthread_mutex_unlock(queue->mutex);
        if (queue->empty_cb != NULL) {
            queue->empty_cb(queue->cb_data);
        }
        return NULL;
    }
    AVPacket *packet = queue->packets[queue->read_index];
    queue->read_index = (queue->read_index + 1) % queue->size;
    queue->count--;
    queue->duration -= packet->duration;
    queue->total_bytes -= packet->size;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
    return packet;
}

void packet_queue_flush(PacketQueue *queue, PacketPool *pool) {
    pthread_mutex_lock(queue->mutex);
    while (queue->count > 0) {
        AVPacket *packet = queue->packets[queue->read_index];
        if (packet != &queue->flush_packet) {
            packet_pool_unref_packet(pool, packet);
        }
        queue->read_index = (queue->read_index + 1) % queue->size;
        queue->count--;
    }
    queue->read_index = 0;
    queue->duration = 0;
    queue->total_bytes = 0;
    queue->packets[0] = &queue->flush_packet;
    queue->write_index = 1;
    queue->count = 1;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
}