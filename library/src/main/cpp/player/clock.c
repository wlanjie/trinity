//
//  clock.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <malloc.h>
#include <sys/time.h>
#include "clock.h"

uint64_t clock_get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

Clock* clock_create() {
    Clock* clock = (Clock*) malloc(sizeof(Clock));
    return clock;
}

int64_t clock_get(Clock *clock) {
    if (clock->update_time == 0) {
        return INT64_MAX;
    }
    return clock->pts + clock_get_current_time() - clock->update_time;
}

void clock_set(Clock *clock, int64_t pts) {
    clock->update_time = clock_get_current_time();
    clock->pts = pts;
}

void clock_free(Clock *clock) {
    free(clock);
}

void clock_reset(Clock *clock) {
    clock->pts = 0;
    clock->update_time = 0;
}