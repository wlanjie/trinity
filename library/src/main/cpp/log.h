//
// Created by wlanjie on 16/4/26.
//

#ifndef STREAMING_LOG_H
#define STREAMING_LOG_H

#include <android/log.h>

#define TAG "trinity_log"

#define LOG

#ifdef LOG
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG ,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG ,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,  __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG ,__VA_ARGS__)
#else
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG , NULL)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG ,NULL)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, NULL)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,  NULL)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG ,NULL)
#endif

#endif //STREAMING_LOG_H
