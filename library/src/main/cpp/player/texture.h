//
//  texture.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//
#ifndef texture_h
#define texture_h
#include "play_types.h"
#include "video_render_types.h"

void initTexture(AVPlayContext * pd);
void texture_delete(AVPlayContext * pd);
void update_texture_yuv420p(Model *model, AVFrame *frame);
void update_texture_nv12(Model *model, AVFrame *frame);
void bind_texture_yuv420p(Model *model);
void bind_texture_nv12(Model *model);
void bind_texture_oes(Model *model);
void update_texture_oes(Model *model, AVFrame *frame);

#endif  // texture_h
