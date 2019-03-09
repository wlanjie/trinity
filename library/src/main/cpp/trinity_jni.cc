//
// Created by wlanjie on 2018/11/2.
//

#include <jni.h>
#include <android/log.h>
#include "log.h"
#include "camera/recording_preview_controller.h"
#include "song_decoder_controller.h"
#include "record_processor.h"
#include "video_packet_consumer.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define RECORD_NAME "com/trinity/camera/PreviewScheduler"
#define MUSIC_DECODE_NAME "com/trinity/decoder/MusicDecoder"
#define AUDIO_RECORD_CONTROLLER_NAME "com/trinity/media/AudioRecordController"
#define AUDIO_RECORD_ECHO_CONTROLLER_NAME "com/trinity/media/AudioRecordEchoController"
#define SOUND_TRACK_CONTROLLER_NAME "com/trinity/media/SoundTrackController"
#define AUDIO_RECORD_PROCESSOR "com/trinity/recording/processor/AudioRecordProcessor"
#define VIDEO_STUDIO "com/trinity/recording/video/VideoRecordingStudio"

//using namespace trinity;

static jlong Android_JNI_createRecord(JNIEnv *env, jobject object) {
    auto *preview_controller = new RecordingPreviewController();
    return reinterpret_cast<jlong>(preview_controller);
}

static void Android_JNI_prepareEGLContext(JNIEnv *env, jobject object, jlong id, jobject surface, jint screen_width,
                                          jint screen_height, jint camera_facing_id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (surface != nullptr && preview_controller != nullptr) {
        JavaVM *vm = nullptr;
        env->GetJavaVM(&vm);
        jobject obj = env->NewGlobalRef(object);
        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        if (window != nullptr) {
            preview_controller->PrepareEGLContext(window, vm, obj, screen_width, screen_height, camera_facing_id);
        }
    }
}

static void Android_JNI_record_switch_camera(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(handle);
    preview_controller->SwitchCameraFacing();
}

static void Android_JNI_createWindowSurface(JNIEnv *env, jobject object, jlong id, jobject surface) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (surface != nullptr && preview_controller != nullptr) {
        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        if (window != nullptr) {
            preview_controller->CreateWindowSurface(window);
        }
    }
}

static void
Android_JNI_adaptiveVideoQuality(JNIEnv *env, jobject object, jlong id, jint max_bit_rate, jint avg_bit_rate,
                                 jint fps) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->AdaptiveVideoQuality(max_bit_rate, avg_bit_rate, fps);
    }
}

static void Android_JNI_hotConfigQuality(JNIEnv *env, jobject object, jlong id, jint max, jint avg, jint fps) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
//        preview_controller->
    }
}

static void Android_JNI_resetRenderSize(JNIEnv *env, jobject object, long id, jint width, jint height) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->ResetRenderSize(width, height);
    }
}

static void Android_JNI_onFrameAvailable(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->NotifyFrameAvailable();
    }
}

static void Android_JNI_updateTextureMatrix(JNIEnv *env, jobject object, jlong id, jfloatArray matrix) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
//        preview_controller->
    }
}

static void Android_JNI_destroyWindowSurface(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->DestroyWindowSurface();
    }
}

static void Android_JNI_destroyEGLContext(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->DestroyEGLContext();
    }
}

static void
Android_JNI_record_start(JNIEnv *env, jobject object, jlong handle, jint width, jint height, jint video_bit_rate,
                         jint frame_rate, jboolean use_hard_encode, jint strategy) {
    if (handle <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(handle);
    preview_controller->StartEncoding(width, height, video_bit_rate, frame_rate, use_hard_encode, strategy);
}

static void Android_JNI_record_stop(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(handle);
    preview_controller->StopEncoding();
}

static void Android_JNI_releaseNative(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<RecordingPreviewController *>(id);
    if (preview_controller != nullptr) {
        delete preview_controller;
    }
}

static jlong Android_JNI_create_music_decoder(JNIEnv *env, jobject object) {
    auto *decoder = new LiveSongDecoderController();
    return reinterpret_cast<jlong>(decoder);
}

static void Android_JNI_musicDecoderInit(JNIEnv *env, jobject object, jlong id, jfloat packetBufferTimePercent,
                                         jint vocalSampleRate) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    decoder->Init(packetBufferTimePercent, vocalSampleRate);
}

static int Android_JNI_music_decoder_read_samples(JNIEnv *env, jobject object, jlong id, jshortArray array, jint size,
                                                  jintArray extraSlientSampleSize, jintArray extraAccompanyType) {
    if (id <= 0) {
        return -1;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    jshort *target = env->GetShortArrayElements(array, 0);
    jint *slientSizeArr = env->GetIntArrayElements(extraSlientSampleSize, 0);
    jint *extraAccompanyTypeArr = env->GetIntArrayElements(extraAccompanyType, 0);
    int result = decoder->ReadSamplesAndProducePacket(target, size, slientSizeArr, extraAccompanyTypeArr);
    env->ReleaseIntArrayElements(extraSlientSampleSize, slientSizeArr, 0);
    env->ReleaseIntArrayElements(extraAccompanyType, extraAccompanyTypeArr, 0);
    env->ReleaseShortArrayElements(array, target, 0);
    return result;
}

static void
Android_JNI_music_decoder_set_volume(JNIEnv *env, jobject object, jlong id, jfloat volume, jfloat accompanyMax) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    decoder->SetVolume(volume, accompanyMax);
}

static void Android_JNI_music_decoder_start(JNIEnv *env, jobject object, jlong id, jstring accompanyFilePathParam) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    const char *accompanyFilePath = env->GetStringUTFChars(accompanyFilePathParam, NULL);
    decoder->StartAccompany(accompanyFilePath);
    env->ReleaseStringUTFChars(accompanyFilePathParam, accompanyFilePath);
}

static void Android_JNI_music_decoder_pause(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    decoder->PauseAccompany();
}

static void Android_JNI_music_decoder_resume(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    decoder->ResumeAccompany();
}

static void Android_JNI_music_decoder_stop(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    decoder->StopAccompany();
}

static void Android_JNI_music_decoder_destory(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    decoder->Destroy();
}

static void Android_JNI_music_decoder_release(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *decoder = reinterpret_cast<LiveSongDecoderController *>(id);
    delete decoder;
}

static jlong
Android_JNI_audio_record_processor_init(JNIEnv *env, jobject object, jint sampleRate, jint audioBufferSize) {
    auto *recorder = new RecordProcessor();
    recorder->InitAudioBufferSize(sampleRate, audioBufferSize);
    return reinterpret_cast<jlong>(recorder);
}

static void Android_JNI_audio_record_processor_flush_audio_buffer_to_queue(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *recorder = reinterpret_cast<RecordProcessor *>(handle);
    recorder->FlushAudioBufferToQueue();
    recorder->Destroy();
    delete recorder;
}

static jint Android_JNI_audio_record_processor_push_audio_buffer_to_queue(JNIEnv *env, jobject object, jlong handle,
                                                                          jshortArray audioSamples,
                                                                          jint audioSampleSize) {
    if (handle <= 0) {
        return -1;
    }
    auto *recorder = reinterpret_cast<RecordProcessor *>(handle);
    jshort *samples = env->GetShortArrayElements(audioSamples, 0);
    int result = recorder->PushAudioBufferToQueue(samples, audioSampleSize);
    env->ReleaseShortArrayElements(audioSamples, samples, 0);
    return result;
}

static void Android_JNI_audio_record_processor_destroy(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *recorder = reinterpret_cast<RecordProcessor *>(handle);
    recorder->Destroy();
    delete recorder;
}

static jlong Android_JNI_video_studio_create(JNIEnv *env, jobject object) {
    auto *videoPacketConsumerThread = new VideoPacketConsumerThread();
    return reinterpret_cast<jlong>(videoPacketConsumerThread);
}

static jint Android_JNI_video_studio_start_record(JNIEnv *env, jobject object, jlong handle, jstring outputPath,
                                                  jint videoWidth, jint videoheight, jint videoFrameRate,
                                                  jint videoBitRate, jint audioSampleRate, jint audioChannels,
                                                  jint audioBitRate,
                                                  jint qualityStrategy) {
    if (handle <= 0) {
        return -1;
    }
    auto *videoPacketConsumerThread = reinterpret_cast<VideoPacketConsumerThread *>(handle);
    int initCode = 0;
    JavaVM *g_jvm = NULL;
    env->GetJavaVM(&g_jvm);
    // TODO delete g_obj
    jobject g_obj = env->NewGlobalRef(object);
    char *videoPath = (char *) (env->GetStringUTFChars(outputPath, NULL));
    //如果视频临时文件存在则删除掉
    remove(videoPath);
    /** 预先初始化3个队列, 防止在init过程中把Statistics重新置为空的问题；
     * 由于先初始化消费者，在初始化生产者，所以队列初始化放到这里 **/
    LiveCommonPacketPool::GetInstance()->InitRecordingVideoPacketQueue();
    LiveCommonPacketPool::GetInstance()->InitAudioPacketQueue(audioSampleRate);
    LiveAudioPacketPool::GetInstance()->InitAudioPacketQueue();

    initCode = videoPacketConsumerThread->Init(videoPath, videoWidth, videoheight, videoFrameRate, videoBitRate,
                                               audioSampleRate,
                                               audioChannels, audioBitRate, "libfdk_aac", qualityStrategy,
                                               g_jvm, g_obj);
    LOGI("initCode is %d...qualityStrategy:%d", initCode, qualityStrategy);
    if (initCode >= 0) {
        videoPacketConsumerThread->StartAsync();
    } else {
        /** 如果初始化失败, 那么就把队列销毁掉 **/
        LiveCommonPacketPool::GetInstance()->DestroyRecordingVideoPacketQueue();
        LiveCommonPacketPool::GetInstance()->DestroyAudioPacketQueue();
        LiveAudioPacketPool::GetInstance()->DestroyAudioPacketQueue();
    }
    env->ReleaseStringUTFChars(outputPath, videoPath);
    return initCode;
}

static void Android_JNI_video_studio_stop_record(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *videoPacketConsumerThread = reinterpret_cast<VideoPacketConsumerThread *>(handle);
    videoPacketConsumerThread->Stop();
}

static void Android_JNI_video_studio_release(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *videoPacketConsumerThread = reinterpret_cast<VideoPacketConsumerThread *>(handle);
    delete videoPacketConsumerThread;
}

static JNINativeMethod recordMethods[] = {
        {"create",               "()J",                           (void **) Android_JNI_createRecord},
        {"prepareEGLContext",    "(JLandroid/view/Surface;III)V", (void **) Android_JNI_prepareEGLContext},
        {"createWindowSurface",  "(JLandroid/view/Surface;)V",    (void **) Android_JNI_createWindowSurface},
        {"switchCamera",         "(J)V",                          (void **) Android_JNI_record_switch_camera},
//        {"adaptiveVideoQuality", "(JIII)V",                       (void **) Android_JNI_adaptiveVideoQuality},
//        {"hotConfigQuality",     "(JIII)V",                       (void **) Android_JNI_hotConfigQuality},
        {"setRenderSize",        "(JII)V",                        (void **) Android_JNI_resetRenderSize},
        {"onFrameAvailable",     "(J)V",                          (void **) Android_JNI_onFrameAvailable},
        {"updateTextureMatrix",  "(J[F)V",                        (void **) Android_JNI_updateTextureMatrix},
        {"destroyWindowSurface", "(J)V",                          (void **) Android_JNI_destroyWindowSurface},
        {"destroyEGLContext",    "(J)V",                          (void **) Android_JNI_destroyEGLContext},
        {"startEncode",          "(JIIIIZI)V",                    (void **) Android_JNI_record_start},
        {"stopEncoding",         "(J)V",                          (void **) Android_JNI_record_stop},
        {"release",              "(J)V",                          (void **) Android_JNI_releaseNative}
};

static JNINativeMethod musicDecoderMethods[] = {
        {"create",         "()J",                    (void **) Android_JNI_create_music_decoder},
        {"initDecoder",    "(JFI)V",                 (void **) Android_JNI_musicDecoderInit},
        {"readSamples",    "(J[SI[I[I)I",            (void **) Android_JNI_music_decoder_read_samples},
        {"setVolume",      "(JFF)V",                 (void **) Android_JNI_music_decoder_set_volume},
        {"start",          "(JLjava/lang/String;)V", (void **) Android_JNI_music_decoder_start},
        {"pause",          "(J)V",                   (void **) Android_JNI_music_decoder_pause},
        {"resume",         "(J)V",                   (void **) Android_JNI_music_decoder_resume},
        {"stop",           "(J)V",                   (void **) Android_JNI_music_decoder_stop},
        {"destroyDecoder", "(J)V",                   (void **) Android_JNI_music_decoder_destory},
        {"release",        "(J)V",                   (void **) Android_JNI_music_decoder_release}
};

//static JNINativeMethod audioRecordControllerMethods[] = {
//        {"createAudioRecorder", "(Ljava/lang/String;)V", (void **) Android_JNI_audio_record_controller_create},
//        {"startRecording",      "()V",                   (void **) Android_JNI_audio_record_controller_start_recording},
//        {"stopRecording",       "()V",                   (void **) Android_JNI_audio_record_controller_stop_recording}
//};

//static JNINativeMethod audioRecordEchoMethods[] = {
//        {"createAudioRecorder", "(Ljava/lang/String;)V", (void **) Android_JNI_audio_record_echo_create},
//        {"startRecording",      "()V",                   (void **) Android_JNI_audio_record_echo_start_recording},
//        {"stopRecording",       "()V",                   (void **) Android_JNI_audio_record_echo_stop_recording}
//};

//static JNINativeMethod soundRecordControllerMethods[] = {
//        {"initMetaData",                  "(IIII)Z", (void **) Anroid_JNI_sound_record_controller_init_meta_data},
//        {"initAudioRecordProcessor",      "(I)Z",    (void **) Android_JNI_sound_record_controller_init_record_processor},
//        {"destroyAudioRecorderProcessor", "()V",     (void **) Android_JNI_sound_record_controller_destroy_processor},
//        {"Start",                         "()V",     (void **) Android_JNI_sound_record_controller_start},
//        {"Stop",                          "()V",     (void **) Android_JNI_sound_record_controller_stop},
//        {"initScoring",                   "(IIII)V", (void **) Android_JNI_sound_record_controller_init_scoring},
//        {"setDestroyScoreProcessorFlag",  "(Z)V",    (void **) Android_JNI_sound_record_controller_score_processor_flag},
//        {"getRenderData",                 "(J[F)V",  (void **) Android_JNI_sound_record_controller_render_data},
//        {"getLatency",                    "(J)I",    (void **) Android_JNI_sound_record_controller_latency},
//        {"getRecordVolume",               "()I",     (void **) Android_JNI_sound_record_controller_record_volume},
//        {"enableUnAccom",                 "()V",     (void **) Android_JNI_sound_record_controller_enable_accom}
//};
//
//static JNINativeMethod soundTrackControllerMethods[] = {
//        {"setAudioDataSource",     "(Ljava/lang/String;F)Z", (void **) Android_JNI_sound_track_set_audio_source},
//        {"getAccompanySampleRate", "()I",                    (void **) Android_JNI_sound_track_get_accompany_sample_rate},
//        {"play",                   "()V",                    (void **) Android_JNI_sound_track_play},
//        {"getCurrentTimeMills",    "()I",                    (void **) Android_JNI_sound_track_get_current_time_mills},
//        {"SetVolume",              "(F)V",                   (void **) Android_JNI_sound_track_set_volume},
//        {"isPlaying",              "()Z",                    (void **) Android_JNI_sound_track_set_is_playing},
//        {"setMusicSourceFlag",     "(Z)V",                   (void **) Android_JNI_sound_track_set_music_source_flag},
//        {"Stop",                   "()V",                    (void **) Android_JNI_sound_track_stop},
//        {"pause",                  "()V",                    (void **) Android_JNI_sound_track_pause},
//        {"seekToPosition",         "(FF)V",                  (void **) Android_JNI_sound_track_seek_to_position}
//};

static JNINativeMethod audioRecordProcessorMethods[] = {
        {"init",                    "(II)J",   (void **) Android_JNI_audio_record_processor_init},
        {"flushAudioBufferToQueue", "(J)V",    (void **) Android_JNI_audio_record_processor_flush_audio_buffer_to_queue},
        {"destroy",                 "(J)V",    (void **) Android_JNI_audio_record_processor_destroy},
        {"pushAudioBufferToQueue",  "(J[SI)I", (void **) Android_JNI_audio_record_processor_push_audio_buffer_to_queue},
};

static JNINativeMethod videoStudioMethods[] = {
        {"create",      "()J",                            (void **) Android_JNI_video_studio_create},
        {"startRecord", "(JLjava/lang/String;IIIIIIII)I", (void **) Android_JNI_video_studio_start_record},
        {"stopRecord",  "(J)V",                           (void **) Android_JNI_video_studio_stop_record},
        {"release",     "(J)V",                           (void **) Android_JNI_video_studio_release}
};

void logCallback(void *ptr, int level, const char *fmt, va_list vl) {
    if (level > av_log_get_level())
        return;
    int android_level = ANDROID_LOG_DEFAULT;
    if (level > AV_LOG_DEBUG) {
        android_level = ANDROID_LOG_DEBUG;
    } else if (level > AV_LOG_VERBOSE) {
        android_level = ANDROID_LOG_VERBOSE;
    } else if (level > AV_LOG_INFO) {
        android_level = ANDROID_LOG_INFO;
    } else if (level > AV_LOG_WARNING) {
        android_level = ANDROID_LOG_WARN;
    } else if (level > AV_LOG_ERROR) {
        android_level = ANDROID_LOG_ERROR;
    } else if (level > AV_LOG_FATAL) {
        android_level = ANDROID_LOG_FATAL;
    } else if (level > AV_LOG_PANIC) {
        android_level = ANDROID_LOG_FATAL;
    } else if (level > AV_LOG_QUIET) {
        android_level = ANDROID_LOG_SILENT;
    }
    __android_log_vprint(android_level, TAG, fmt, vl);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD("JNI_OnLoad");
    JNIEnv *env = NULL;
    if ((vm)->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass record = env->FindClass(RECORD_NAME);
    env->RegisterNatives(record, recordMethods, NELEM(recordMethods));
    env->DeleteLocalRef(record);

    jclass musicDecoder = env->FindClass(MUSIC_DECODE_NAME);
    env->RegisterNatives(musicDecoder, musicDecoderMethods, NELEM(musicDecoderMethods));
    env->DeleteLocalRef(musicDecoder);

    jclass audioRecordProcessor = env->FindClass(AUDIO_RECORD_PROCESSOR);
    env->RegisterNatives(audioRecordProcessor, audioRecordProcessorMethods, NELEM(audioRecordProcessorMethods));
    env->DeleteLocalRef(audioRecordProcessor);

    jclass videoStudio = env->FindClass(VIDEO_STUDIO);
    env->RegisterNatives(videoStudio, videoStudioMethods, NELEM(videoStudioMethods));
    env->DeleteLocalRef(videoStudio);

    av_register_all();
    avcodec_register_all();
#ifdef VIDEO_DEBUG
    av_log_set_callback(logCallback);
#endif
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
}

#ifdef __cplusplus
}
#endif