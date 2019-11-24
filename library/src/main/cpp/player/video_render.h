//
//  video_render.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef video_render_h
#define video_render_h

#include "play_types.h"
void video_render_ctx_reset(VideoRenderContext *video_render_context);
VideoRenderContext* video_render_ctx_create();
void video_render_ctx_release(VideoRenderContext *video_render_context);

#endif  // video_render_h
