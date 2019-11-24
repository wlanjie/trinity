//
//  audio_filter.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef audio_filter_h
#define audio_filter_h

#include "play_types.h"

AudioFilterContext* audio_filter_create();
void audio_filter_release(AudioFilterContext* audio_filter_context);
void audio_filter_change_speed(AVPlayContext* context, float speed);
#endif  // audio_filter_h
