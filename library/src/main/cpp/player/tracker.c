//
//  tracker.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <android/sensor.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/prctl.h>
#include "tracker.h"
#include "mat4.h"

typedef struct xl_ekf_context_struct {
    ASensorManager *sensor_manager;
    ASensor const *acc;
    ASensor const *gyro;
    int acc_min_delay, gyro_min_delay;
    ALooper *looper;
    ASensorEventQueue *event_queue;
    pthread_mutex_t * lock;
    struct timeval last_gyro_ts;
} xl_ekf_context;

static xl_ekf_context * c;
static bool run = false;
static pthread_t ekf_tid;
static const int LOOPER_ID_USER = 3;
// 横屏  Matrix.setRotateEulerM(mEkfToHeadTracker, 0, -90.0F, 0.0F, 0.0F);
static float ekf_to_head_tracker[16] = {
    -1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -4.371139E-8f, 1.0f, 0.0f,
    0.0f, 1.0f, -4.371139E-8f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};


xl_ekf_context *xl_ekf_context_create() {
    xl_ekf_context *ctx = (xl_ekf_context *) malloc(sizeof(xl_ekf_context));
    memset(ctx, 0, sizeof(xl_ekf_context));
//    ctx->sensor_manager = ASensorManager_getInstanceForPackage("com.cls.xl.xl");
    ctx->sensor_manager = ASensorManager_getInstance();
    ctx->acc = ASensorManager_getDefaultSensor(ctx->sensor_manager, ASENSOR_TYPE_ACCELEROMETER);
    ctx->gyro = ASensorManager_getDefaultSensor(ctx->sensor_manager, ASENSOR_TYPE_GYROSCOPE);
    ctx->acc_min_delay = ASensor_getMinDelay(ctx->acc);
    ctx->gyro_min_delay = ASensor_getMinDelay(ctx->gyro);
    if (ctx->acc == NULL || ctx->gyro == NULL) {
        free(ctx);
        return NULL;
    }
    ctx->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ctx->event_queue = ASensorManager_createEventQueue(
            ctx->sensor_manager,
            ctx->looper,
            LOOPER_ID_USER,
            NULL,
            NULL
    );
    ASensorEventQueue_enableSensor(ctx->event_queue, ctx->acc);
    ASensorEventQueue_enableSensor(ctx->event_queue, ctx->gyro);
    int acc_delay = 20000;
    int gyro_delay = 20000;
    acc_delay = acc_delay > ctx->acc_min_delay ? acc_delay : ctx->acc_min_delay;
    gyro_delay = gyro_delay > ctx->gyro_min_delay ? gyro_delay : ctx->gyro_min_delay;
    ASensorEventQueue_setEventRate(ctx->event_queue, ctx->acc, acc_delay);
    ASensorEventQueue_setEventRate(ctx->event_queue, ctx->gyro, gyro_delay);
    ctx->lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(ctx->lock, NULL);
    return ctx;
}

void xl_ekf_context_release(xl_ekf_context * ctx) {
    ASensorManager_destroyEventQueue(ctx->sensor_manager ,ctx->event_queue);
    pthread_mutex_destroy(ctx->lock);
    free(ctx->lock);
    free(ctx);
}

void *ekf_thread(__attribute__((unused)) void *data) {
    prctl(PR_SET_NAME, __func__);
    LOGI("head tracker thread start");
    xl_ekf_context * ctx = xl_ekf_context_create();
    if(ctx == NULL){
        return NULL;
    }
    c = ctx;
//    xl_ekf_reset();
//    int ident;
//    int events;
//    struct android_poll_source* source;
//    while(run){
//        while (run && (ident=ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {
//            if(ident < 0){
//                LOGI("looper poll all  ident < 0");
//            }
//            if (ident == LOOPER_ID_USER) {
//                ASensorEvent event;
//                while (ASensorEventQueue_getEvents(ctx->event_queue,
//                                                   &event, 1) > 0) {
//                    if (event.type == ASENSOR_TYPE_ACCELEROMETER) {
//                        pthread_mutex_lock(ctx->lock);
//                        xl_ekf_process_acc(
//                                -event.data[1],
//                                event.data[0],
//                                event.data[2],
//                                event.timestamp
//                        );
//                        pthread_mutex_unlock(ctx->lock);
//                    } else if (event.type == ASENSOR_TYPE_GYROSCOPE) {
//                        pthread_mutex_lock(ctx->lock);
//                        gettimeofday(&ctx->last_gyro_ts, NULL);
//                        xl_ekf_process_gyro(
//                                -event.data[1],
//                                event.data[0],
//                                event.data[2],
//                                event.timestamp
//                        );
//                        pthread_mutex_unlock(ctx->lock);
//                    }
//                }
//            }
//        }
//    }
//    c = NULL;
//    xl_ekf_context_release(ctx);
//    LOGI("head tracker thread exit");
    return NULL;
}


void tracker_start() {
    if(run) return;
    run = true;
    pthread_create(&ekf_tid, NULL, ekf_thread, NULL);
}

void tracker_stop() {
    if(!run) return;
    if(c != NULL){
        ASensorEventQueue_disableSensor(c->event_queue, c->acc);
        ASensorEventQueue_disableSensor(c->event_queue, c->gyro);
    }
    run = false;
    if(c != NULL){
        ALooper_wake(c->looper);
    }
    pthread_join(ekf_tid, NULL);
}

void tracker_get_last_view(float *matrix){
    if(c == NULL){
        identity(matrix);
        return;
    }
    struct timeval now;
    gettimeofday(&now, NULL);
    double time_diff_nano = (double)now.tv_sec + (double)now.tv_usec / 1000000.0
                                - (double)(c->last_gyro_ts.tv_sec)
                                - (double)(c->last_gyro_ts.tv_usec) / 1000000.0
                                + 0.03333333333333333;
    pthread_mutex_lock(c->lock);
//    xl_ekf_get_predicted_matrix(time_diff_nano, matrix);
    pthread_mutex_unlock(c->lock);
    multiply(matrix, matrix, ekf_to_head_tracker);
}