//
//  video_render.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <pthread.h>
#include "video_render.h"

void video_render_set_window(VideoRenderContext* video_render_context, struct ANativeWindow *window) {
    pthread_mutex_lock(video_render_context->lock);
    video_render_context->window = window;
    video_render_context->cmd |= CMD_SET_WINDOW;
    pthread_mutex_unlock(video_render_context->lock);
}

void video_render_change_model(VideoRenderContext* video_render_context, ModelType model_type) {
    pthread_mutex_lock(video_render_context->lock);
    video_render_context->require_model_type = model_type;
    video_render_context->cmd |= CMD_CHANGE_MODEL;
    pthread_mutex_unlock(video_render_context->lock);
}

void video_render_ctx_reset(VideoRenderContext* video_render_context){
    video_render_context->surface = EGL_NO_SURFACE;
    video_render_context->context = EGL_NO_CONTEXT;
    video_render_context->display = EGL_NO_DISPLAY;
    video_render_context->model = NULL;
    video_render_context->enable_tracker = false;
    video_render_context->require_model_type = NONE;
    video_render_context->require_model_scale = 1;
    video_render_context->cmd = NO_CMD;
    video_render_context->enable_tracker = false;
    video_render_context->draw_mode = wait_frame;
    video_render_context->require_model_rotation[0] = 0;
    video_render_context->require_model_rotation[1] = 0;
    video_render_context->require_model_rotation[2] = 0;
    video_render_context->width = video_render_context->height = 1;
}


VideoRenderContext* video_render_ctx_create() {
    VideoRenderContext* video_render_context = malloc(sizeof(VideoRenderContext));
    video_render_context->lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(video_render_context->lock, NULL);
    video_render_context->set_window = video_render_set_window;
    video_render_context->change_model = video_render_change_model;
    video_render_context->window = NULL;
    video_render_context->texture_window = NULL;
    video_render_ctx_reset(video_render_context);
    return video_render_context;
}

void video_render_ctx_release(VideoRenderContext* video_render_context) {
    pthread_mutex_destroy(video_render_context->lock);
    free(video_render_context->lock);
    free(video_render_context);
}