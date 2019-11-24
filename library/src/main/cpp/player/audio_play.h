//
//  audio_play.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef audio_play_h
#define audio_play_h

#include "play_types.h"

AudioPlayContext* audio_engine_create();
void audio_player_create(int rate, int channel, AVPlayContext *context);

#endif  // audio_play_h
