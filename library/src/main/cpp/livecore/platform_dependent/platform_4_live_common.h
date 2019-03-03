/*
 * code here will be compiled according to platforms, for android and IOS, please make your own implement
 */

#ifndef PLATFORM_4_LIVE_COMMON
#define PLATFORM_4_LIVE_COMMON

#include <string>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

typedef unsigned char byte;

#ifndef UINT64_C
#define UINT64_C        uint64_t
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

namespace platform_4_live {
static inline long getCurrentTimeMills()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline long getCurrentTimeSeconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

}

// log
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#elif defined(__APPLE__)	// IOS or OSX
#if (LIVEVIDEO_DEBUG > 0)
#define LOGI(...)  printf("  ");printf(__VA_ARGS__); printf("\t -  <%s> \n", LOG_TAG);
#define LOGE(...)  printf(" Error: ");printf(__VA_ARGS__); printf("\t -  <%s> \n", LOG_TAG);
#else
#define LOGI(...) {};
#define LOGE(...) {};
#endif
#endif



#endif	// PLATFORM_4_LIVE_COMMON


