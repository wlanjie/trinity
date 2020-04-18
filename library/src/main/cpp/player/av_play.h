//
//  av_play.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef av_play_h
#define av_play_h

#include <pthread.h>
#include "clock.h"
#include "media_codec.h"
#include "play_types.h"

AVPlayContext* av_play_create(JNIEnv *env, jobject instance, int play_create,
                             int sample_rate);

void av_play_set_buffer_time(AVPlayContext *context, float buffer_time);

void av_play_set_buffer_size(AVPlayContext *context, int buffer_size);

int av_play_play(const char *url, float time, AVPlayContext *context);

int av_play_resume(AVPlayContext *context);

void av_play_pause(AVPlayContext* context);

void av_play_seek(AVPlayContext *context, float seek_to);

void av_play_set_play_background(AVPlayContext *context, bool play_background);

int av_play_stop(AVPlayContext *context);

int av_play_release(AVPlayContext *context);

void change_status(AVPlayContext *context, PlayStatus statuse);

#endif  // av_play_h
