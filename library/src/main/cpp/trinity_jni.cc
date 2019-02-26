//
// Created by wlanjie on 2018/11/2.
//

#include <jni.h>
#include <android/log.h>
#include "log.h"
#include "record/preview_controller.h"

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

#define RECORD_NAME "com/trinity/Record"

using namespace trinity;

static jlong Android_JNI_createRecord(JNIEnv *env, jobject object) {
    auto *preview_controller = new PreviewController();
    return reinterpret_cast<jlong>(preview_controller);
}

static void Android_JNI_prepareEGLContext(JNIEnv *env, jobject object, jlong id, jobject surface, jint screen_width,
                                          jint screen_height, jint camera_facing_id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
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

static void Android_JNI_createWindowSurface(JNIEnv *env, jobject object, jlong id, jobject surface) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
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
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->AdaptiveVideoQuality(max_bit_rate, avg_bit_rate, fps);
    }
}

static void Android_JNI_hotConfigQuality(JNIEnv *env, jobject object, jlong id, jint max, jint avg, jint fps) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
//        preview_controller->
    }
}

static void Android_JNI_resetRenderSize(JNIEnv *env, jobject object, long id, jint width, jint height) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->ResetRenderSize(width, height);
    }
}

static void Android_JNI_onFrameAvailable(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->NotifyFrameAvailable();
    }
}

static void Android_JNI_updateTextureMatrix(JNIEnv *env, jobject object, jlong id, jfloatArray matrix) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
//        preview_controller->
    }
}

static void Android_JNI_destroyWindowSurface(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->DestroyWindowSurface();
    }
}

static void Android_JNI_destroyEGLContext(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
        preview_controller->DestroyEGLContext();
    }
}

static void Android_JNI_releaseNative(JNIEnv *env, jobject object, jlong id) {
    if (id <= 0) {
        return;
    }
    auto *preview_controller = reinterpret_cast<PreviewController *>(id);
    if (preview_controller != nullptr) {
        delete preview_controller;
    }
}

static JNINativeMethod recordMethods[] = {
        {"createNative",         "()J",                           (void **) Android_JNI_createRecord},
        {"prepareEGLContext",    "(JLandroid/view/Surface;III)V", (void **) Android_JNI_prepareEGLContext},
        {"createWindowSurface",  "(JLandroid/view/Surface;)V",    (void **) Android_JNI_createWindowSurface},
        {"adaptiveVideoQuality", "(JIII)V",                       (void **) Android_JNI_adaptiveVideoQuality},
        {"hotConfigQuality",     "(JIII)V",                       (void **) Android_JNI_hotConfigQuality},
        {"resetRenderSize",      "(JII)V",                        (void **) Android_JNI_resetRenderSize},
        {"onFrameAvailable",     "(J)V",                          (void **) Android_JNI_onFrameAvailable},
        {"updateTextureMatrix",  "(J[F)V",                        (void **) Android_JNI_updateTextureMatrix},
        {"destroyWindowSurface", "(J)V",                          (void **) Android_JNI_destroyWindowSurface},
        {"destroyEGLContext",    "(J)V",                          (void **) Android_JNI_destroyEGLContext},
        {"releaseNative",        "(J)V",                          (void **) Android_JNI_releaseNative}
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