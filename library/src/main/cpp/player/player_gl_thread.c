//
//  player_gl_thread.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <unistd.h>
#include "player_gl_thread.h"
#include "play_types.h"
#include "av_play.h"
#include "model.h"
#include "queue.h"
#include "glsl_program.h"
#include "tracker.h"
#include "texture.h"
#include <sys/prctl.h>

static void init_egl(AVPlayContext * pd){
    VideoRenderContext *ctx = pd->video_render_context;
    const EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE,
                              8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
                              EGL_NONE};
    EGLint numConfigs;
    ctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint majorVersion, minorVersion;
    eglInitialize(ctx->display, &majorVersion, &minorVersion);
    eglChooseConfig(ctx->display, attribs, &ctx->config, 1, &numConfigs);
    ctx->surface = eglCreateWindowSurface(ctx->display, ctx->config, ctx->window, NULL);
    EGLint attrs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    ctx->context = eglCreateContext(ctx->display, ctx->config, NULL, attrs);
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS) {
        LOGE("egl error");
    }
    if (eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context) == EGL_FALSE) {
        LOGE("------EGL-FALSE");
    }
    eglQuerySurface(ctx->display, ctx->surface, EGL_WIDTH, &ctx->width);
    eglQuerySurface(ctx->display, ctx->surface, EGL_HEIGHT, &ctx->height);
    initTexture(pd);
    if(!pd->is_sw_decode) {
        JavaClass * jc = pd->java_class;
        JNIEnv * jniEnv = pd->video_render_context->env;
        jobject surface_texture = (*jniEnv)->CallStaticObjectMethod(jniEnv, jc->SurfaceTextureBridge, jc->texture_getSurface, ctx->texture[3]);
        ctx->texture_window = ANativeWindow_fromSurface(jniEnv, surface_texture);
    }

}

static void release_egl(AVPlayContext * pd) {
    VideoRenderContext *ctx = pd->video_render_context;
    if (ctx->display == EGL_NO_DISPLAY) return;
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(ctx->display, ctx->surface);
    texture_delete(pd);
    glsl_program_clear_all();
    if(ctx->texture_window != NULL){
        ANativeWindow_release(ctx->texture_window);
        ctx->texture_window = NULL;
    }
    if (ctx->model != NULL) {
        freeModel(ctx->model);
        ctx->model = NULL;
    }
    eglMakeCurrent(ctx->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (ctx->context != EGL_NO_CONTEXT) {
        eglDestroyContext(ctx->display, ctx->context);
    }
    if (ctx->surface != EGL_NO_SURFACE) {
        eglDestroySurface(ctx->display, ctx->surface);
    }
    eglTerminate(ctx->display);
    ctx->display = EGL_NO_DISPLAY;
    ctx->context = EGL_NO_CONTEXT;
    ctx->surface = EGL_NO_SURFACE;
}

static void set_window(AVPlayContext * pd) {
    VideoRenderContext *ctx = pd->video_render_context;
    if(ctx->display == EGL_NO_DISPLAY){
        init_egl(pd);
    }else {
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(ctx->display, ctx->surface);
        eglDestroySurface(ctx->display, ctx->surface);

        ctx->surface = eglCreateWindowSurface(ctx->display, ctx->config, ctx->window, NULL);
        EGLint err = eglGetError();
        if (err != EGL_SUCCESS) {
            LOGE("egl error");
        }
        if (eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context) == EGL_FALSE) {
            LOGE("------EGL-FALSE");
        }
        eglQuerySurface(ctx->display, ctx->surface, EGL_WIDTH, &ctx->width);
        eglQuerySurface(ctx->display, ctx->surface, EGL_HEIGHT, &ctx->height);
    }
}

void change_model(VideoRenderContext *video_render_context) {
    if (video_render_context->model != NULL) {
        freeModel(video_render_context->model);
    }
    video_render_context->model = createModel(video_render_context->require_model_type);
    memcpy(video_render_context->model->texture, video_render_context->texture, sizeof(GLuint) * 4);
    video_render_context->model->resize(video_render_context->model, video_render_context->width, video_render_context->height);
    if (video_render_context->model->type == Ball) {
        video_render_context->draw_mode = fixed_frequency;
        tracker_start();
    } else {
        tracker_stop();
        video_render_context->draw_mode = wait_frame;
    }
}

static inline void player_release_video_frame(AVPlayContext *pd, AVFrame *frame) {
    if (!pd->is_sw_decode) {
        mediacodec_release_buffer(pd, frame);
    }
    frame_pool_unref_frame(pd->video_frame_pool, frame);
    pd->video_frame = NULL;
}

static inline void draw_now(VideoRenderContext *ctx) {
    Model *model = ctx->model;
    pthread_mutex_lock(ctx->lock);
    if (ctx->draw_mode == fixed_frequency && ctx->enable_tracker) {
        tracker_get_last_view(model->head_matrix);
        model->updateHead(model, model->head_matrix);
    }
    // The initialization is done
    if (model->pixel_format != AV_PIX_FMT_NONE) {
        model->draw(model);
    }
    eglSwapBuffers(ctx->display, ctx->surface);
    pthread_mutex_unlock(ctx->lock);
}

/**
 *
 * @param pd
 * @param frame
 * @return  0   draw
 *         -1   sleep 33ms  continue
 *         -2   break
 */
static inline int draw_video_frame(AVPlayContext *pd) {
    // 上一次可能没有画， 这种情况就不需要取新的了
    if (pd->video_frame == NULL) {
        pd->video_frame = frame_queue_get(pd->video_frame_queue);
    }
    // buffer empty  ==> sleep 10ms , return 0
    // eos           ==> return -2
    if (pd->video_frame == NULL) {
        if (pd->eof) {
            return -2;
        } else {
            usleep(BUFFER_EMPTY_SLEEP_US);
            return 0;
        }
    }

    // 发现seeking == 2 开始drop frame 直到发现&pd->flush_frame
    // 解决连续seek卡住的问题
    if (pd->seeking == 2) {
        while (pd->video_frame != &pd->video_frame_queue->flush_frame) {
            player_release_video_frame(pd, pd->video_frame);
            pd->video_frame = frame_queue_get(pd->video_frame_queue);
            if (pd->video_frame == NULL) {
                return 0;
            }
        }
        pd->seeking = 0;
        pd->video_frame = NULL;
        clock_reset(pd->video_clock);
        return 0;
    }
    int64_t time_stamp;
    if (pd->is_sw_decode) {
        time_stamp = av_rescale_q(pd->video_frame->pts,
                                  pd->format_context->streams[pd->video_index]->time_base,
                                  AV_TIME_BASE_Q);
    } else {
        time_stamp = pd->video_frame->pts;
    }


    int64_t diff = 0;
    if(pd->av_track_flags & AUDIO_FLAG){
        diff = time_stamp - (pd->audio_clock->pts + pd->audio_player_context->get_delta_time(pd->audio_player_context));
    }else{
        diff = time_stamp - clock_get(pd->video_clock);
    }
    Model* model = pd->video_render_context->model;


    // diff >= 33ms if draw_mode == wait_frame return -1
    //              if draw_mode == fixed_frequency draw previous frame ,return 0
    // diff > 0 && diff < 33ms  sleep(diff) draw return 0
    // diff <= 0  draw return 0
    if (diff >= WAIT_FRAME_SLEEP_US) {
        if (pd->video_render_context->draw_mode == wait_frame) {
            LOGE("wait_frame");
            return -1;
        } else {
            draw_now(pd->video_render_context);
            return 0;
        }
    } else {
        // if diff > WAIT_FRAME_SLEEP_US   then  use previous frame
        // else  use current frame   and  release frame
        pthread_mutex_lock(pd->video_render_context->lock);
        model->update_frame(model, pd->video_frame);
        pthread_mutex_unlock(pd->video_render_context->lock);
        player_release_video_frame(pd, pd->video_frame);
        if(!pd->is_sw_decode){
            JNIEnv* jniEnv = pd->video_render_context->env;
            (*jniEnv)->CallStaticVoidMethod(jniEnv, pd->java_class->SurfaceTextureBridge, pd->java_class->texture_updateTexImage);
            jfloatArray texture_matrix_array = (*jniEnv)->CallStaticObjectMethod(jniEnv, pd->java_class->SurfaceTextureBridge, pd->java_class->texture_getTransformMatrix);
            (*jniEnv)->GetFloatArrayRegion(jniEnv, texture_matrix_array, 0, 16, model->texture_matrix);
            (*jniEnv)->DeleteLocalRef(jniEnv, texture_matrix_array);
        }

        if (diff > 0) {
            usleep((useconds_t) diff);
        }
        draw_now(pd->video_render_context);
        pd->statistics->frames ++;
        clock_set(pd->video_clock, time_stamp);
        return 0;
    }
}

void *player_gl_thread(void *data) {
    prctl(PR_SET_NAME, __func__);
    AVPlayContext *pd = (AVPlayContext *) data;
    VideoRenderContext *ctx = pd->video_render_context;
    (*pd->vm)->AttachCurrentThread(pd->vm, &ctx->env, NULL);
    int ret;
    while (pd->error_code == 0) {
        // 处理egl
        pthread_mutex_lock(ctx->lock);
        if (ctx->cmd != NO_CMD) {
            if ((ctx->cmd & CMD_SET_WINDOW) != 0) {
                set_window(pd);
                change_model(ctx);
            }
            if ((ctx->cmd & CMD_CHANGE_MODEL) != 0) {
                if (ctx->display == EGL_NO_DISPLAY) {
                    LOGE("window: %p", ctx->window);
                    if (ctx->window != NULL) {
                        set_window(pd);
                    } else {
                        pthread_mutex_unlock(ctx->lock);
                        usleep(NULL_LOOP_SLEEP_US);
                        continue;
                    }
                }
                change_model(ctx);
            }
            // TODO
            if ((ctx->cmd & CMD_CHANGE_SCALE) != 0 && ctx->model != NULL && ctx->model->update_distance != NULL) {
                ctx->model->update_distance(ctx->model, ctx->require_model_scale);
            }
            if ((ctx->cmd & CMD_CHANGE_ROTATION) != 0 && ctx->model != NULL && ctx->model->updateModelRotation != NULL) {
                ctx->model->updateModelRotation(ctx->model, ctx->require_model_rotation[0],
                                                ctx->require_model_rotation[1],
                                                ctx->require_model_rotation[2],ctx->enable_tracker);
            }
            ctx->cmd = NO_CMD;
        }
        pthread_mutex_unlock(ctx->lock);
        // 处理pd->status
        if (pd->status == PAUSED || pd->status == BUFFER_EMPTY) {
            usleep(NULL_LOOP_SLEEP_US);
        } else if (pd->status == PLAYING) {
            ret = draw_video_frame(pd);
            if (ret == 0) {
                continue;
            } else if (ret == -1) {
                usleep(WAIT_FRAME_SLEEP_US);
                continue;
            } else if (ret == -2) {
                // 如果有视频   就在这发结束信号
                pd->send_message(pd, message_stop);
                break;
            }
        } else if (pd->status == IDEL) {
            usleep(WAIT_FRAME_SLEEP_US);
        } else {
            LOGE("error state  ==> %d", pd->status);
            break;
        }
    }
    release_egl(pd);
    tracker_stop();
    (*pd->vm)->DetachCurrentThread(pd->vm);
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}