//
//  clock.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef clock_h
#define clock_h


#include <stdint.h>
#include "play_types.h"

uint64_t clock_get_current_time();
Clock* clock_create();
int64_t clock_get(Clock *clock);
void clock_set(Clock *clock, int64_t pts);
void clock_free(Clock *clock);
void clock_reset(Clock *clock);
#endif  // clock_h
