//
//  audio_play.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "audio_play.h"
#include "queue.h"
#include "clock.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <assert.h>
#include <pthread.h>

// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLVolumeItf bqPlayerVolume;


static int64_t get_delta_time(AudioPlayContext * context) {
    SLmillisecond sec;
    (*bqPlayerPlay)->GetPosition(bqPlayerPlay, &sec);
    if (sec > context->play_pos) {
        return (int64_t) (sec - context->play_pos) * 1000;
    }
    return 0;
}

static int get_audio_frame(AVPlayContext* context) {
    AudioPlayContext* audio_play_context = NULL;
    if (context->status == IDEL || context->status == PAUSED || context->status == BUFFER_EMPTY) {
        return -1;
    }
    context->audio_frame = frame_queue_get(context->audio_frame_queue);
    // buffer empty ==> return -1
    // eos          ==> return -1
    if (context->audio_frame == NULL) {
        // 如果没有视频流  就从这里发结束信号
        if (context->eof && (((context->av_track_flags & VIDEO_FLAG) == 0) || context->just_audio)) {
            context->send_message(context, message_stop);
        }
        return -1;
    }
    // seek
    // get next frame
    while (context->audio_frame == &context->audio_frame_queue->flush_frame) {
        // 如果没有视频流  就从这里重置seek标记
        if((context->av_track_flags & VIDEO_FLAG) == 0){
            context->seeking = 0;
        }
        return get_audio_frame(context);
    }

    audio_play_context->frame_size = av_samples_get_buffer_size(NULL, context->audio_frame->channels, context->audio_frame->nb_samples,
                                                     AV_SAMPLE_FMT_S16, 1);
    // filter will rewrite the frame's pts.  use  ptk_dts instead.
    int64_t time_stamp = av_rescale_q(context->audio_frame->pkt_dts,
                                      context->format_context->streams[context->audio_index]->time_base,
                                      AV_TIME_BASE_Q);
    if(audio_play_context->buffer_size < audio_play_context->frame_size){
        audio_play_context->buffer_size = audio_play_context->frame_size;
        if (audio_play_context->buffer == NULL) {
            audio_play_context->buffer = malloc((size_t) audio_play_context->buffer_size);
        } else {
            audio_play_context->buffer = realloc(audio_play_context->buffer, (size_t) audio_play_context->buffer_size);
        }
    }
    if(audio_play_context->frame_size > 0){
        memcpy(audio_play_context->buffer, context->audio_frame->data[0], (size_t) audio_play_context->frame_size);
    }
    frame_pool_unref_frame(context->audio_frame_pool, context->audio_frame);
    clock_set(context->audio_clock, time_stamp);
    return 0;
}

static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *args) {
    AVPlayContext* context = args;
    AudioPlayContext* ctx = NULL;
    pthread_mutex_lock(ctx->lock);
    assert(bq == bqPlayerBufferQueue);
    if (-1 == get_audio_frame(context)) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);
        pthread_mutex_unlock(ctx->lock);
        return;
    }
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (NULL != ctx->buffer && 0 != ctx->frame_size) {
        SLresult result;
        // enqueue another buffer
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, ctx->buffer,
                                                 (SLuint32) ctx->frame_size);
        (*bqPlayerPlay)->GetPosition(bqPlayerPlay, &ctx->play_pos);
        (void) result;
    }
    pthread_mutex_unlock(ctx->lock);
}

void audio_play(AVPlayContext *pd) {
    SLresult result = 0;
    (*bqPlayerPlay)->GetPlayState(bqPlayerPlay, &result);
    if(result == SL_PLAYSTATE_PAUSED){
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
        bqPlayerCallback(bqPlayerBufferQueue, pd);
    }
}

void audio_player_shutdown();

void audio_player_release(AudioPlayContext *audio_play_context);

void audio_player_create(int rate, int channel, AVPlayContext *context);

AudioPlayContext* audio_engine_create() {
    LOGE("audio_engine_create");
    SLresult result;
    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // create output mix, with environmental reverb specified as a non-required interface
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example

    AudioPlayContext * ctx = malloc(sizeof(AudioPlayContext));
    ctx->play = audio_play;
    ctx->shutdown = audio_player_shutdown;
    ctx->release = audio_player_release;
    ctx->player_create = audio_player_create;
    ctx->get_delta_time = get_delta_time;
    ctx->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    ctx->buffer_size = 0;
    ctx->buffer = NULL;
    pthread_mutex_init(ctx->lock, NULL);
    return ctx;
}

void audio_player_create(int rate, int channel, AVPlayContext *context) {
    LOGI("enter %s rate: %d channel: %d", __func__, rate, channel);
    SLresult result;
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    /**
     *  SLuint32 		formatType;
	 *  SLuint32 		numChannels;
	 *  SLuint32 		samplesPerSec;
	 *  SLuint32 		bitsPerSample;
	 *  SLuint32 		containerSize;
	 *  SLuint32 		channelMask;
	 *  SLuint32		endianness;
     */
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    format_pcm.numChannels = (SLuint32) channel;
    format_pcm.samplesPerSec = (SLuint32) (rate * 1000);
    if (channel == 2) {
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    } else {
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    }
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
                                                2, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, context);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // get the playbackRate interface
//    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAYBACKRATE, &playbackRateItf);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    LOGI("leave %s", __func__);
}

void audio_player_shutdown() {
    LOGI("enter %s", __func__);
    if(bqPlayerPlay != NULL){
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    }
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerVolume = NULL;
    }
    LOGI("leave %s", __func__);
}

void audio_player_release(AudioPlayContext *audio_play_context) {
    LOGI("enter %s", __func__);
    // destroy buffer queue audio player object, and invalidate all associated interfaces

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }
    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    if (audio_play_context->lock != NULL) {
        pthread_mutex_destroy(audio_play_context->lock);
        free(audio_play_context->lock);
    }
    if (audio_play_context->buffer != NULL) {
        free(audio_play_context->buffer);
    }
    free(audio_play_context);
    LOGI("leave %s", __func__);
}