//
//  statistics.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "statistics.h"
#include "clock.h"

Statistics *statistics_create(JNIEnv* env) {
    Statistics *statistics = malloc(sizeof(Statistics));
    statistics->ret_buffer = malloc(12);
    jobject buf = (*env)->NewDirectByteBuffer(env, statistics->ret_buffer, 12);
    statistics->ret_buffer_java = (*env)->NewGlobalRef(env, buf);
    (*env)->DeleteLocalRef(env, buf);
    return statistics;
}

void statistics_reset(Statistics *statistics) {
    statistics->last_update_time = 0;
    statistics->last_update_bytes = 0;
    statistics->last_update_frames = 0;
    statistics->bytes = 0;
    statistics->frames = 0;
}

void statistics_release(JNIEnv* env, Statistics *statistics) {
    (*env)->DeleteGlobalRef(env, statistics->ret_buffer_java);
    free(statistics->ret_buffer);
    free(statistics);
}

void statistics_update(Statistics *statistics) {
    statistics->last_update_time = clock_get_current_time();
    statistics->last_update_bytes = statistics->bytes;
    statistics->last_update_frames = statistics->frames;
}

int statistics_get_fps(Statistics *statistics) {
    int64_t frames = statistics->frames - statistics->last_update_frames;
    int64_t time_diff = clock_get_current_time() - statistics->last_update_time;
    return (int) (frames * 1000000 / time_diff);
}

int statistics_get_bps(Statistics *statistics) {
    int64_t bytes = statistics->bytes - statistics->last_update_bytes;
    int64_t time_diff = clock_get_current_time() - statistics->last_update_time;
    return (int) (bytes * 8 * 1000000 / time_diff);
}