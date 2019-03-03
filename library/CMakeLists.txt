# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(PATH_TO_MEDIACORE ${CMAKE_SOURCE_DIR}/src/main/cpp)
set(PATH_TO_JNI_LAYER ${CMAKE_SOURCE_DIR}/src/main/jni)
set(PATH_TO_THIRDPARTY ${PATH_TO_MEDIACORE}/thirdparty)
set(PATH_TO_PRE_BUILT ${PATH_TO_THIRDPARTY}/prebuilt/${ANDROID_ABI})

add_definitions("
-DGL_GLEXT_PROTOTYPES
-DEGL_EGLEXT_PROTOTYPES
")

include_directories(BEFORE ${PATH_TO_THIRDPARTY}/ffmpeg/include/)
include_directories(${PATH_TO_MEDIACORE}/)
include_directories(${PATH_TO_MEDIACORE}/common/)
include_directories(${PATH_TO_MEDIACORE}/camera/)
include_directories(${PATH_TO_MEDIACORE}/livecore/)
include_directories(${PATH_TO_MEDIACORE}/livecore/common/)
include_directories(${PATH_TO_MEDIACORE}/livecore/audio_encoder/)
include_directories(${PATH_TO_MEDIACORE}/livecore/consumer/)
include_directories(${PATH_TO_MEDIACORE}/livecore/platform_dependent/)
include_directories(${PATH_TO_MEDIACORE}/livecore/mp4/)
include_directories(${PATH_TO_MEDIACORE}/decoder/)
include_directories(${PATH_TO_MEDIACORE}/processor/)
include_directories(${PATH_TO_MEDIACORE}/video_consumer/)
include_directories(${PATH_TO_MEDIACORE}/video_consumer/libaudio_encoder/)
include_directories(${PATH_TO_MEDIACORE}/video_consumer/libmusic_merger/)
include_directories(${PATH_TO_MEDIACORE}/video_consumer/libvideo_consumer/)
include_directories(${PATH_TO_MEDIACORE}/encoder/)
include_directories(${PATH_TO_MEDIACORE}/encoder/soft_encoder/)
include_directories(${PATH_TO_MEDIACORE}/encoder/hw_encoder/)

file(GLOB FILES_TEST "${PATH_TO_MEDIACORE}/*.cpp")
file(GLOB FILES_JNI_LAYER "${PATH_TO_JNI_LAYER}/*.cpp")
file(GLOB FILES_LIB_COMMON "${PATH_TO_MEDIACORE}/common/*.cpp")
file(GLOB FILES_LIB_COMMON_EGL_CORE "${PATH_TO_MEDIACORE}/common/egl_core/*.cpp")
file(GLOB FILES_LIB_COMMON_EGL_CORE "${PATH_TO_MEDIACORE}/common/egl_core/*.cpp")
file(GLOB FILES_LIB_COMMON_RESAMPLER "${PATH_TO_MEDIACORE}/common/libresampler/*.cpp")
file(GLOB FILES_LIB_COMMON_MSG_Q "${PATH_TO_MEDIACORE}/common/message_queue/*.cpp")
file(GLOB FILES_LIB_COMMON_OPENGL_MEDIA "${PATH_TO_MEDIACORE}/common/opengl_media/*.cpp")
file(GLOB FILES_LIB_COMMON_OPENGL_MEDIA_RENDER "${PATH_TO_MEDIACORE}/common/opengl_media/render/*.cpp")
file(GLOB FILES_LIB_COMMON_OPENGL_MEDIA_TEXTURE "${PATH_TO_MEDIACORE}/common/opengl_media/texture/*.cpp")
file(GLOB FILES_LIB_COMMON_OPENGL_MEDIA_TEX_COPIER "${PATH_TO_MEDIACORE}/common/opengl_media/texture_copier/*.cpp")
file(GLOB FILES_LIB_CAMERA_PREVIEW "${PATH_TO_MEDIACORE}/camera/*.cpp")
file(GLOB FILES_LIB_LIVE_CORE_COMMON "${PATH_TO_MEDIACORE}/livecore/common/*.cpp")
file(GLOB FILES_LIB_LIVE_CORE_AUDIOENCODER "${PATH_TO_MEDIACORE}/livecore/audio_encoder/*.cpp")
file(GLOB FILES_LIB_LIVE_CORE_CONSUMER "${PATH_TO_MEDIACORE}/livecore/consumer/*.cpp")
file(GLOB FILES_LIB_LIVE_CORE_PUBLISHER "${PATH_TO_MEDIACORE}/livecore/mp4/*.cpp")
file(GLOB FILES_LIB_MUSIC_DECODER "${PATH_TO_MEDIACORE}/decoder/*.cpp")
file(GLOB FILES_LIB_RECORDER_PROCESSOR "${PATH_TO_MEDIACORE}/processor/*.cpp")
file(GLOB FILES_LIB_VIDEO_CONSUMER_A_ENCODER "${PATH_TO_MEDIACORE}/video_consumer/libaudio_encoder/*.cpp")
file(GLOB FILES_LIB_VIDEO_CONSUMER_MUSIC_MERGER "${PATH_TO_MEDIACORE}/video_consumer/libmusic_merger/*.cpp")
file(GLOB FILES_LIB_VIDEO_CONSUMER "${PATH_TO_MEDIACORE}/video_consumer/libvideo_consumer/*.cpp")
file(GLOB FILES_LIB_VIDEO_ENCODER "${PATH_TO_MEDIACORE}/encoder/*.cpp")
file(GLOB FILES_LIB_VIDEO_ENCODER_HW "${PATH_TO_MEDIACORE}/encoder/hw_encoder/*.cpp")
file(GLOB FILES_LIB_VIDEO_ENCODER_X264 "${PATH_TO_MEDIACORE}/encoder/soft_encoder/*.cpp")
file(GLOB FILES_LIB_VIDEO_ENCODER_SOFT_COLOR "${PATH_TO_MEDIACORE}/encoder/soft_encoder/color_conversion/color_conversion_armv7a.cpp")

add_library(native-lib SHARED
        ${FILES_TEST}
        ${FILES_JNI_LAYER}
        ${FILES_LIB_COMMON}
        ${FILES_LIB_COMMON_EGL_CORE}
        ${FILES_LIB_COMMON_MSG_Q}
        ${FILES_LIB_COMMON_RESAMPLER}
        ${FILES_LIB_COMMON_OPENGL_MEDIA}
        ${FILES_LIB_COMMON_OPENGL_MEDIA_RENDER}
        ${FILES_LIB_COMMON_OPENGL_MEDIA_TEXTURE}
        ${FILES_LIB_COMMON_OPENGL_MEDIA_TEX_COPIER}
        ${FILES_LIB_CAMERA_PREVIEW}
        ${FILES_LIB_LIVE_CORE_COMMON}
        ${FILES_LIB_LIVE_CORE_AUDIOENCODER}
        ${FILES_LIB_LIVE_CORE_CONSUMER}
        ${FILES_LIB_LIVE_CORE_PUBLISHER}
        ${FILES_LIB_MUSIC_DECODER}
        ${FILES_LIB_RECORDER_PROCESSOR}
        ${FILES_LIB_VIDEO_CONSUMER_A_ENCODER}
        ${FILES_LIB_VIDEO_CONSUMER_MUSIC_MERGER}
        ${FILES_LIB_VIDEO_CONSUMER}
        ${FILES_LIB_VIDEO_ENCODER}
        ${FILES_LIB_VIDEO_ENCODER_HW}
        ${FILES_LIB_VIDEO_ENCODER_X264}
        ${FILES_LIB_VIDEO_ENCODER_SOFT_COLOR}
        )

target_link_libraries(native-lib
                       # 引入系统的动态库
                       log
                       android
                       GLESv2
                       EGL
                       z
                       OpenSLES
                       # 引入ffmpeg相关静态库
                       ${PATH_TO_PRE_BUILT}/libavfilter.a
                       ${PATH_TO_PRE_BUILT}/libavformat.a
                       ${PATH_TO_PRE_BUILT}/libavcodec.a
                       ${PATH_TO_PRE_BUILT}/libpostproc.a
                       ${PATH_TO_PRE_BUILT}/libswresample.a
                       ${PATH_TO_PRE_BUILT}/libswscale.a
                       ${PATH_TO_PRE_BUILT}/libavutil.a
                       ${PATH_TO_PRE_BUILT}/libpostproc.a
                       ${PATH_TO_PRE_BUILT}/libfdk-aac.a
                       ${PATH_TO_PRE_BUILT}/libvo-aacenc.a
                       ${PATH_TO_PRE_BUILT}/libx264.a
                       ${PATH_TO_PRE_BUILT}/libsox.a
                       )