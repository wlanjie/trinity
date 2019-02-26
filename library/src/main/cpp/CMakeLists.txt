cmake_minimum_required(VERSION 3.4.1)

enable_language(ASM)

set(-DCMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -s")
set(-DCMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -s")

add_definitions("
-DGL_GLEXT_PROTOTYPES
-DEGL_EGLEXT_PROTOTYPES
")

#if (${ANDROID_PLATFORM_LEVEL} LESS 18)
    add_definitions(-D_OPENGL20_)
    set(OPENGL_LIB GLESv2)
#else()
#    set(OPENGL_LIB GLESv3)
#    add_definitions(-D_OPENGL30_)
#endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DVIDEO_DEBUG)
else()
endif()

# include ffmpeg header
set(FFMPEG_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/ffmpeg/${ANDROID_ABI})
include_directories(${FFMPEG_HEADER})
include_directories(${FFMPEG_HEADER}/include)

#include x264 header
set(X264_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/libx264/${ANDROID_ABI}/include)
include_directories(${X264_HEADER})

#include fdk-aac header
set(FDKAAC_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/fdk-aac/include)
include_directories(${FDKAAC_HEADER})

#include jpeg-turbo header
set(JPEG_TURBO_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/jpeg-turbo/${ANDROID_ABI}/include)
include_directories(${JPEG_TURBO_HEADER})

add_library(
        trinity
        SHARED
        trinity_jni.cc
        record/preview_controller.cc
        record/preview_renderer.cc
        common/matrix.cc
        common/message_queue/handler.cc
        common/message_queue/message_queue.cc
        common/egl/egl_core.cc
        common/egl/egl_share_context.cc
        common/opengl_media/texture_frame.cc
        common/opengl_media/gpu_texture_frame.cc
        common/texture_copier/gpu_texture_frame_copier.cc
        common/texture_copier/texture_frame_copier.cc
        common/texture_copier/yuv_texture_frame_copier.cc
        common/opengl_media/render/video_gl_surface_render.cc)

set(FFMPEG_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/ffmpeg/${ANDROID_ABI})
add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavcodec.a)

add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavformat.a)

add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libavutil.a)

add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/lib/libswresample.a)

set(JPEG_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/jpeg-turbo/${ANDROID_ABI}/lib/libjpeg.a)
add_library(jpeg STATIC IMPORTED)
set_target_properties(jpeg PROPERTIES IMPORTED_LOCATION ${JPEG_LIB})

set(JPEG_TURBO_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/jpeg-turbo/${ANDROID_ABI}/lib/libjpeg.a)
add_library(jpeg_turbo STATIC IMPORTED)
set_target_properties(jpeg_turbo PROPERTIES IMPORTED_LOCATION ${JPEG_TURBO_LIB})

target_link_libraries(trinity android jnigraphics log z swresample avformat avcodec avutil jpeg jpeg_turbo GLESv2 EGL OpenSLES)