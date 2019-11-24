//
//  statistics.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef statistics_h
#define statistics_h
#include "play_types.h"

Statistics* statistics_create(JNIEnv* env);
void statistics_release(JNIEnv* env, Statistics *statistics);
void statistics_reset(Statistics *statistics);
void statistics_update(Statistics *statistics);
int statistics_get_fps(Statistics *statistics);
int statistics_get_bps(Statistics *statistics);

#endif  // statistics_h
