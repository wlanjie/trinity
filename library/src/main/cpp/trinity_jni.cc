//
// Created by wlanjie on 2018/11/2.
//

#include <jni.h>
#include <android/log.h>
#include "android_xlog.h"
#include "camera_record.h"
#include "record_processor.h"
#include "music_decoder_controller.h"
#include "audio_render.h"
#include "video_editor.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
}

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "trinity"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define RECORD_NAME "com/trinity/record/TrinityRecord"
#define MUSIC_DECODE_NAME "com/trinity/decoder/MusicDecoder"
#define AUDIO_RECORD_CONTROLLER_NAME "com/trinity/media/AudioRecordController"
#define AUDIO_RECORD_ECHO_CONTROLLER_NAME "com/trinity/media/AudioRecordEchoController"
#define SOUND_TRACK_CONTROLLER_NAME "com/trinity/media/SoundTrackController"
#define AUDIO_RECORD_PROCESSOR "com/trinity/record/processor/AudioRecordProcessor"
#define AUDIO_PLAYER "com/trinity/player/AudioPlayer"
#define VIDEO_EDITOR "com/trinity/editor/VideoEditor"

using namespace trinity;

static jlong Android_JNI_createRecord(JNIEnv *env, jobject object) {
    auto *record = new CameraRecord();
    return reinterpret_cast<jlong>(record);
}

static void Android_JNI_prepareEGLContext(JNIEnv *env, jobject object, jlong id, jobject surface, jint screen_width,
                                          jint screen_height, jint camera_facing_id) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (surface != nullptr && record != nullptr) {
        JavaVM *vm = nullptr;
        env->GetJavaVM(&vm);
        jobject obj = env->NewGlobalRef(object);
        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        if (window != nullptr) {
            record->PrepareEGLContext(window, vm, obj, screen_width, screen_height, camera_facing_id);
        }
    }
}

static void Android_JNI_record_switch_camera(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(handle);
    record->SwitchCameraFacing();
}

static void Android_JNI_createWindowSurface(JNIEnv *env, jobject object, jlong id, jobject surface) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (surface != nullptr && record != nullptr) {
        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        if (window != nullptr) {
            record->CreateWindowSurface(window);
        }
    }
}

static void Android_JNI_resetRenderSize(JNIEnv *env, jobject object, long id, jint width, jint height) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
        record->ResetRenderSize(width, height);
    }
}

static void Android_JNI_onFrameAvailable(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
        record->NotifyFrameAvailable();
    }
}

static void Android_JNI_setFrame(JNIEnv* env, jobject object, jlong id, jint frame) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
        record->SetFrame(frame);
    }
}

static void Android_JNI_updateTextureMatrix(JNIEnv *env, jobject object, jlong id, jfloatArray matrix) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
//        preview_controller->
    }
}

static void Android_JNI_destroyWindowSurface(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
        record->DestroyWindowSurface();
    }
}

static void Android_JNI_destroyEGLContext(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
        record->DestroyEGLContext();
    }
}

static void
Android_JNI_record_start(JNIEnv *env, jobject object, jlong handle, jstring path, jint width, jint height, jint video_bit_rate,
                         jint frame_rate, jboolean use_hard_encode,
                         jint audio_sample_rate, jint audio_channel, jint audio_bit_rate) {
    if (handle <= 0) {
        return;
    }
    const char* output = env->GetStringUTFChars(path, JNI_FALSE);
    auto *record = reinterpret_cast<CameraRecord *>(handle);
    record->StartEncoding(
            output,
            width, height, video_bit_rate, frame_rate,
            use_hard_encode,
            audio_sample_rate, audio_channel, audio_bit_rate);
    env->ReleaseStringUTFChars(path, output);
}

static void Android_JNI_record_stop(JNIEnv *env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<CameraRecord *>(handle);
    preview_controller->StopEncoding();
}

static void Android_JNI_releaseNative(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *record = reinterpret_cast<CameraRecord *>(id);
    if (record != nullptr) {
        delete record;
    }
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

static jlong Android_JNI_audio_player_create(JNIEnv* env, jobject object) {
    MusicDecoderController* controller = new MusicDecoderController();
    return reinterpret_cast<jlong>(controller);
}

static void Android_JNI_audio_player_start(JNIEnv* env, jobject object, jlong id, jstring path) {
    if (id <= 0) {
        return;
    }
    const char* file_path = env->GetStringUTFChars(path, JNI_FALSE);
    MusicDecoderController* controller = reinterpret_cast<MusicDecoderController*>(id);
    controller->Init(0.2f, 44100);
    controller->Start(file_path);
    env->ReleaseStringUTFChars(path, file_path);
}

static void Android_JNI_audio_player_resume(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    MusicDecoderController* controller = reinterpret_cast<MusicDecoderController*>(id);
    controller->Resume();
}

static void Android_JNI_audio_player_pause(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    MusicDecoderController* controller = reinterpret_cast<MusicDecoderController*>(id);
    controller->Pause();
}

static void Android_JNI_audio_player_stop(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    MusicDecoderController* controller = reinterpret_cast<MusicDecoderController*>(id);
    controller->Stop();
}

static void Android_JNI_audio_player_release(JNIEnv* env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    MusicDecoderController* controller = reinterpret_cast<MusicDecoderController*>(id);
    delete controller;
}

// video editor
static jlong Android_JNI_video_editor_create(JNIEnv* env, jobject object) {
    VideoEditor* editor = new VideoEditor();
    editor->Init();
    return reinterpret_cast<jlong>(editor);
}

static void
Android_JNI_video_editor_surfaceCreated(JNIEnv *env, jobject object, jlong handle, jobject surface, jint width,
                                         jint height) {
    if (handle <= 0) {
        return;
    }
    VideoEditor *editor = reinterpret_cast<VideoEditor *>(handle);
    editor->OnSurfaceCreated(env, object, surface);
}

static void Android_JNI_video_editor_surfaceChanged(JNIEnv* env, jobject object, jlong handle, jint width, jint height) {
    if (handle <= 0) {
        return;
    }
    VideoEditor *editor = reinterpret_cast<VideoEditor *>(handle);
    editor->OnSurfaceChanged(width, height);
}

static void Android_JNI_video_editor_surfaceDestroyed(JNIEnv *env, jobject object, jlong handle, jobject surface) {
    if (handle <= 0) {
        return;
    }
    VideoEditor *editor = reinterpret_cast<VideoEditor *>(handle);
    editor->OnSurfaceDestroyed(env);
}

static jlong Android_JNI_video_editor_getVideoDuration(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return editor->GetVideoDuration();
}

static int Android_JNI_video_editor_getClipsCount(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return editor->GetClipsCount();
}

static jobject Android_JNI_video_editor_getClip(JNIEnv* env, jobject object, jlong handle, jint index) {
    if (handle <= 0) {
        return nullptr;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
//    return editor->GetClip(index);
    return nullptr;
}

static int Android_JNI_video_editor_insertClip(JNIEnv* env, jobject object, jlong handle, jobject clip) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    jclass clip_clazz = env->GetObjectClass(clip);
    jfieldID path_field_id = env->GetFieldID(clip_clazz, "path", "Ljava/lang/String;");
    jstring path_string = static_cast<jstring>(env->GetObjectField(clip, path_field_id));
    const char* media_path = env->GetStringUTFChars(path_string, JNI_FALSE);
    char* file_copy = new char[strlen(media_path) + 1];
    sprintf(file_copy, "%s%c", media_path, 0);
    MediaClip* media_clip = new MediaClip();
    media_clip->file_name = file_copy;
    int result = editor->InsertClip(media_clip);
    env->ReleaseStringUTFChars(path_string, media_path);
    return result;
}

static int Android_JNI_video_editor_insertClipWithIndex(JNIEnv* env, jobject object, jlong handle, jint index, jobject clip) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return 0;
}

static void Android_JNI_video_editor_removeClip(JNIEnv* env, jobject object, jlong handle, jint index) {
    if (handle <= 0) {
        return;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    editor->RemoveClip(index);
}

static int Android_JNI_video_editor_replaceClip(JNIEnv* env, jobject object, jlong handle, jint index, jobject clip) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return 0;
}

static jobject Android_JNI_video_editor_getVideoClips(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return nullptr;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return nullptr;
}

static jobject Android_JNI_video_editor_getClipTimeRange(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return nullptr;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return nullptr;
}

static jlong Android_JNI_video_editor_getVideoTime(JNIEnv* env, jobject object, jlong handle, jint index, jlong clip_time) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return editor->GetVideoTime(index, clip_time);
}

static jlong Android_JNI_video_editor_getClipTime(JNIEnv* env, jobject object, jlong handle, jint index, jlong video_time) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return editor->GetClipTime(index, video_time);
}

static jint Android_JNI_video_editor_getClipIndex(JNIEnv* env, jobject object, jlong handle, jlong time) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return editor->GetClipIndex(time);
}

static jint Android_JNI_video_editor_addFilter(JNIEnv* env, jobject object, jlong handle, jbyteArray lut, jlong start_time, jlong end_time, jint action_id) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    jbyte* lut_buffer = env->GetByteArrayElements(lut, JNI_FALSE);
    int lut_size = env->GetArrayLength(lut);
    int id = editor->AddFilter(reinterpret_cast<uint8_t *>(lut_buffer), lut_size, start_time, end_time, action_id);
    env->ReleaseByteArrayElements(lut, lut_buffer, 0);
    return id;
}

static jint Android_JNI_video_editor_addMusic(JNIEnv* env, jobject object, jlong handle, jstring music_path, jlong start_time, jlong end_time) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    const char* path = env->GetStringUTFChars(music_path, JNI_FALSE);
    int result = editor->AddMusic(path, start_time, end_time);
    env->ReleaseStringUTFChars(music_path, path);
    return result;
}

static int Android_JNI_video_editor_play(JNIEnv* env, jobject object, jlong handle, jboolean repeat) {
    if (handle <= 0) {
        return 0;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    return editor->Play(repeat, env, object);
}

static void Android_JNI_video_editor_pause(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    editor->Pause();
}

static void Android_JNI_video_editor_resume(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    editor->Resume();
}

static void Android_JNI_video_editor_stop(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    editor->Stop();
}

static void Android_JNI_video_editor_release(JNIEnv* env, jobject object, jlong handle) {
    if (handle <= 0) {
        return;
    }
    VideoEditor* editor = reinterpret_cast<VideoEditor*>(handle);
    editor->Destroy();
    delete editor;
}

static JNINativeMethod recordMethods[] = {
        {"create",               "()J",                             (void **) Android_JNI_createRecord},
        {"prepareEGLContext",    "(JLandroid/view/Surface;III)V",   (void **) Android_JNI_prepareEGLContext},
        {"createWindowSurface",  "(JLandroid/view/Surface;)V",      (void **) Android_JNI_createWindowSurface},
        {"switchCamera",         "(J)V",                            (void **) Android_JNI_record_switch_camera},
        {"setRenderSize",        "(JII)V",                          (void **) Android_JNI_resetRenderSize},
        {"onFrameAvailable",     "(J)V",                            (void **) Android_JNI_onFrameAvailable},
        {"setFrame",             "(JI)V",                           (void **) Android_JNI_setFrame },
        {"updateTextureMatrix",  "(J[F)V",                          (void **) Android_JNI_updateTextureMatrix},
        {"destroyWindowSurface", "(J)V",                            (void **) Android_JNI_destroyWindowSurface},
        {"destroyEGLContext",    "(J)V",                            (void **) Android_JNI_destroyEGLContext},
        {"startEncode",          "(JLjava/lang/String;IIIIZIII)V",  (void **) Android_JNI_record_start},
        {"stopEncode",           "(J)V",                            (void **) Android_JNI_record_stop},
        {"release",              "(J)V",                            (void **) Android_JNI_releaseNative}
};

static JNINativeMethod audioRecordProcessorMethods[] = {
        {"init",                    "(II)J",   (void **) Android_JNI_audio_record_processor_init},
        {"flushAudioBufferToQueue", "(J)V",    (void **) Android_JNI_audio_record_processor_flush_audio_buffer_to_queue},
        {"destroy",                 "(J)V",    (void **) Android_JNI_audio_record_processor_destroy},
        {"pushAudioBufferToQueue",  "(J[SI)I", (void **) Android_JNI_audio_record_processor_push_audio_buffer_to_queue},
};

static JNINativeMethod audioPlayerMethods[] = {
        {"create",                 "()J",                    (void **) Android_JNI_audio_player_create },
        {"start",                  "(JLjava/lang/String;)I", (void **) Android_JNI_audio_player_start },
        {"resume",                 "(J)V",                   (void **) Android_JNI_audio_player_resume },
        {"pause",                  "(J)V",                   (void **) Android_JNI_audio_player_pause },
        {"stop",                   "(J)V",                   (void **) Android_JNI_audio_player_stop },
        {"release",                "(J)V",                   (void **) Android_JNI_audio_player_release }
};

static JNINativeMethod videoEditorMethods[] = {
        {"create",              "()J",                                                   (void **) Android_JNI_video_editor_create },
        {"onSurfaceCreated",    "(JLandroid/view/Surface;)V",                            (void **) Android_JNI_video_editor_surfaceCreated},
        {"onSurfaceDestroyed",  "(JLandroid/view/Surface;)V",                            (void **) Android_JNI_video_editor_surfaceDestroyed},
        {"onSurfaceChanged",    "(JII)V",                                                (void **) Android_JNI_video_editor_surfaceChanged },
        {"getVideoDuration",    "(J)J",                                                  (void **) Android_JNI_video_editor_getVideoDuration },
        {"getClipsCount",       "(J)I",                                                  (void **) Android_JNI_video_editor_getClipsCount },
        {"getClip",             "(JI)Lcom/trinity/editor/MediaClip;",                    (void **) Android_JNI_video_editor_getClip },
        {"insertClip",          "(JLcom/trinity/editor/MediaClip;)I",                    (void **) Android_JNI_video_editor_insertClip },
        {"insertClip",          "(JILcom/trinity/editor/MediaClip;)I",                   (void **) Android_JNI_video_editor_insertClipWithIndex },
        {"removeClip",          "(JI)V",                                                 (void **) Android_JNI_video_editor_removeClip },
        {"replaceClip",         "(JILcom/trinity/editor/MediaClip;)I",                   (void **) Android_JNI_video_editor_replaceClip },
        {"getVideoClips",       "(J)Ljava/util/List;",                                   (void **) Android_JNI_video_editor_getVideoClips },
        {"getClipTimeRange",    "(JI)Lcom/trinity/editor/TimeRange;",                    (void **) Android_JNI_video_editor_getClipTimeRange },
        {"getVideoTime",        "(JIJ)J",                                                (void **) Android_JNI_video_editor_getVideoTime },
        {"getClipTime",         "(JIJ)J",                                                (void **) Android_JNI_video_editor_getClipTime },
        {"getClipIndex",        "(JJ)I",                                                 (void **) Android_JNI_video_editor_getClipIndex },
        {"addFilter",           "(J[BJJI)I",                                             (void **) Android_JNI_video_editor_addFilter },
        {"addMusic",            "(JLjava/lang/String;JJ)I",                              (void **) Android_JNI_video_editor_addMusic },
        {"play",                "(JZ)I",                                                 (void **) Android_JNI_video_editor_play },
        {"pause",               "(J)V",                                                  (void **) Android_JNI_video_editor_pause },
        {"resume",              "(J)V",                                                  (void **) Android_JNI_video_editor_resume },
        {"stop",                "(J)V",                                                  (void **) Android_JNI_video_editor_stop },
        {"release",             "(J)V",                                                  (void **) Android_JNI_video_editor_release }
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
//    __android_log_vprint(android_level, TAG, fmt, vl);
//    __android_log_print(ANDROID_LOG_INFO, TAG, fmt, vl);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if ((vm)->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass record = env->FindClass(RECORD_NAME);
    env->RegisterNatives(record, recordMethods, NELEM(recordMethods));
    env->DeleteLocalRef(record);

    jclass audioRecordProcessor = env->FindClass(AUDIO_RECORD_PROCESSOR);
    env->RegisterNatives(audioRecordProcessor, audioRecordProcessorMethods, NELEM(audioRecordProcessorMethods));
    env->DeleteLocalRef(audioRecordProcessor);

    jclass audioPlayer = env->FindClass(AUDIO_PLAYER);
    env->RegisterNatives(audioPlayer, audioPlayerMethods, NELEM(audioPlayerMethods));
    env->DeleteLocalRef(audioPlayer);

    jclass videoEditor = env->FindClass(VIDEO_EDITOR);
    env->RegisterNatives(videoEditor, videoEditorMethods, NELEM(videoEditorMethods));
    env->DeleteLocalRef(videoEditor);

    av_register_all();
    avcodec_register_all();
    avfilter_register_all();
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