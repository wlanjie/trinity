cmake_minimum_required(VERSION 3.4.1)

enable_language(ASM)

set(-DCMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -s -Wdeprecated-declarations")
set(-DCMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -s -Wdeprecated-declarations")
set(PATH_TO_MEDIACORE ${CMAKE_SOURCE_DIR})

add_definitions("
-DGL_GLEXT_PROTOTYPES
-DEGL_EGLEXT_PROTOTYPES
")

if (${ANDROID_PLATFORM_LEVEL} LESS 18)
    add_definitions(-D_OPENGL20_)
    set(OPENGL_LIB GLESv2)
else()
    set(OPENGL_LIB GLESv3)
    add_definitions(-D_OPENGL30_)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DVIDEO_DEBUG)
else()
endif()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${PATH_TO_MEDIACORE}/)
include_directories(${PATH_TO_MEDIACORE}/camera/)
include_directories(${PATH_TO_MEDIACORE}/message/)
include_directories(${PATH_TO_MEDIACORE}/egl/)
include_directories(${PATH_TO_MEDIACORE}/opengl/)
include_directories(${PATH_TO_MEDIACORE}/encode/)
include_directories(${PATH_TO_MEDIACORE}/encode/color_conversion/)
include_directories(${PATH_TO_MEDIACORE}/json/)
include_directories(${PATH_TO_MEDIACORE}/decode/)
include_directories(${PATH_TO_MEDIACORE}/queue/)
include_directories(${PATH_TO_MEDIACORE}/thread/)
include_directories(${PATH_TO_MEDIACORE}/mp4/)
include_directories(${PATH_TO_MEDIACORE}/util/)
include_directories(${PATH_TO_MEDIACORE}/player/)
include_directories(${PATH_TO_MEDIACORE}/opensl/)
include_directories(${PATH_TO_MEDIACORE}/editor/)
include_directories(${PATH_TO_MEDIACORE}/stb/)

# include ffmpeg header
set(FFMPEG_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/ffmpeg/${ANDROID_ABI}/include)
include_directories(${FFMPEG_HEADER})

#include x264 header
set(X264_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/libx264/${ANDROID_ABI}/include)
include_directories(${X264_HEADER})

#include fdk-aac header
set(FDKAAC_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/fdk-aac/${ANDROID_ABI}/include)
include_directories(${FDKAAC_HEADER})

#include jpeg-turbo header
#set(JPEG_TURBO_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/jpeg-turbo/${ANDROID_ABI}/include)
#include_directories(${JPEG_TURBO_HEADER})

#include xlog
set(XLOG_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/xlogger/include)
include_directories(${XLOG_HEADER})

#include glm
set(GLM_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/glm/glm)
include_directories(${GLM_HEADER})
include_directories(${GLM_HEADER}/gtc)
include_directories(${GLM_HEADER}/gtx)

file(GLOB FILES "${PATH_TO_MEDIACORE}/*.cc")
file(GLOB FILES_C "${PATH_TO_MEDIACORE}/*.c")
file(GLOB FILES_CAMERA "${PATH_TO_MEDIACORE}/camera/*.cc")
file(GLOB FILES_MESSAGE "${PATH_TO_MEDIACORE}/message/*.cc")
file(GLOB FILES_EGL "${PATH_TO_MEDIACORE}/egl/*.cc")
file(GLOB FILES_OPENGL "${PATH_TO_MEDIACORE}/opengl")
file(GLOB FILES_ENCODE "${PATH_TO_MEDIACORE}/encode/*.cc")
file(GLOB FILES_JSON "${PATH_TO_MEDIACORE}/json/*.c")
file(GLOB FILES_DECODE "${PATH_TO_MEDIACORE}/decode/*.cc")
file(GLOB FILES_DECODE_C "${PATH_TO_MEDIACORE}/decode/*.c")
file(GLOB FILES_QUEUE "${PATH_TO_MEDIACORE}/queue/*.cc")
file(GLOB FILES_THREAD "${PATH_TO_MEDIACORE}/thread/*.cc")
file(GLOB FILES_MP4 "${PATH_TO_MEDIACORE}/mp4/*.cc")
file(GLOB FILES_UTIL "${PATH_TO_MEDIACORE}/util/*.cc")
file(GLOB FILES_UTIL_C "${PATH_TO_MEDIACORE}/util/*.c")
file(GLOB FILES_PLAYER "${PATH_TO_MEDIACORE}/player/*.cc")
file(GLOB FILES_PLAYER_C "${PATH_TO_MEDIACORE}/player/*.c")
file(GLOB FILE_OPENSL "${PATH_TO_MEDIACORE}/opensl/*.cc")
file(GLOB FILE_VIDEO_EDITOR "${PATH_TO_MEDIACORE}/editor/*.cc")

add_library(
        trinity
        SHARED
        trinity_jni.cc
        ${FILES}
        ${FILES_C}
        ${FILES_CAMERA}
        ${FILES_MESSAGE}
        ${FILES_EGL}
        ${FILES_ENCODE}
        ${FILES_JSON}
        ${FILES_DECODE}
        ${FILES_DECODE_C}
        ${FILES_QUEUE}
        ${FILES_THREAD}
        ${FILES_MP4}
        ${FILES_UTIL}
        ${FILES_UTIL_C}
        ${FILES_PLAYER}
        ${FILES_PLAYER_C}
        ${FILE_OPENSL}
        ${FILE_VIDEO_EDITOR}
        ${FILES_OPENGL}/encode_render.cc
        ${FILES_OPENGL}/frame_buffer.cc
        ${FILES_OPENGL}/gaussian_blur.cc
        ${FILES_OPENGL}/opengl.cc
        ${FILES_OPENGL}/yuv_render.cc
        ${FILES_OPENGL}/image_process.cc
        ${FILES_OPENGL}/process_buffer.cc
        ${FILES_OPENGL}/effect.cc
        ${FILES_OPENGL}/blend.cc
        ${FILES_OPENGL}/face_markup_render.cc)

set(FFMPEG_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/ffmpeg/${ANDROID_ABI})
add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavcodec.a)

add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavformat.a)

add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavutil.a)

add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libswresample.a)

add_library(avfilter STATIC IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavfilter.a)

set(FDK_AAC ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/fdk-aac/${ANDROID_ABI}/lib/libfdk-aac.a)
add_library(fdk-aac STATIC IMPORTED)
set_target_properties(fdk-aac PROPERTIES IMPORTED_LOCATION ${FDK_AAC})

set(X264 ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/libx264/${ANDROID_ABI}/lib/libx264.a)
add_library(x264 STATIC IMPORTED)
set_target_properties(x264 PROPERTIES IMPORTED_LOCATION ${X264})

#set(JPEG_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/jpeg-turbo/${ANDROID_ABI}/lib/libjpeg.a)
#add_library(jpeg STATIC IMPORTED)
#set_target_properties(jpeg PROPERTIES IMPORTED_LOCATION ${JPEG_LIB})
#
#set(JPEG_TURBO_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/jpeg-turbo/${ANDROID_ABI}/lib/libjpeg.a)
#add_library(jpeg_turbo STATIC IMPORTED)
#set_target_properties(jpeg_turbo PROPERTIES IMPORTED_LOCATION ${JPEG_TURBO_LIB})

set(XLOG_LIB ${CMAKE_CURRENT_LIST_DIR}/../jniLibs/${ANDROID_ABI}/libmarsxlog.so)
add_library(xlog SHARED IMPORTED)
set_target_properties(xlog PROPERTIES IMPORTED_LOCATION ${XLOG_LIB})

#target_link_libraries(trinity android jnigraphics log z swresample avformat avcodec avutil jpeg jpeg_turbo GLESv2 EGL OpenSLES)
if (${CMAKE_SYSTEM_VERSION} MATCHES "21")
    target_link_libraries(trinity
        log
        android
        GLESv2
        EGL
        z
        mediandk
        OpenSLES
        swresample
        avformat
        avcodec
        avfilter
        avutil
        fdk-aac
        x264
        xlog)
else (${CMAKE_SYSTEM_VERSION} LESS "21")
    target_link_libraries(trinity
        log
        android
        GLESv2
        EGL
        z
        OpenSLES
        swresample
        avformat
        avcodec
        avfilter
        avutil
        fdk-aac
        x264
        xlog)
endif (${CMAKE_SYSTEM_VERSION} MATCHES "21")