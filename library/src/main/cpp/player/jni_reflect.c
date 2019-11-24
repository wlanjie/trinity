//
//  jni_reflect.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "jni_reflect.h"

void jni_reflect_java_class(JavaClass ** p_jc, JNIEnv *env) {
    JavaClass* java_class = malloc(sizeof(JavaClass));
    jclass player_class = (*env)->FindClass(env, "com/trinity/editor/VideoEditor");
    java_class->player_onPlayStatusChanged = (*env)->GetMethodID(env, player_class,
                                                     "onPlayStatusChanged", "(I)V");
    java_class->player_onPlayError = (*env)->GetMethodID(env, player_class,
                                                     "onPlayError", "(I)V");
//    java_class->XLPlayer_class = (*env)->NewGlobalRef(env, player_class);
    (*env)->DeleteLocalRef(env, player_class);

    jclass java_HwDecodeBridge = (*env)->FindClass(env, "com/trinity/decoder/HwDecodeBridge");
    java_class->codec_init = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "init", "(Ljava/lang/String;IILjava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    java_class->codec_stop = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "stop", "()V");
    java_class->codec_flush = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "flush",  "()V");
    java_class->codec_dequeueInputBuffer = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "dequeueInputBuffer", "(J)I");
    java_class->codec_queueInputBuffer = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "queueInputBuffer", "(IIJI)V");
    java_class->codec_getInputBuffer = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "getInputBuffer", "(I)Ljava/nio/ByteBuffer;");
    java_class->codec_getOutputBuffer = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "getOutputBuffer", "(I)Ljava/nio/ByteBuffer;");
    java_class->codec_releaseOutPutBuffer = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "releaseOutPutBuffer",  "(I)V");
    java_class->codec_release = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "release", "()V");
    java_class->codec_formatChange = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "formatChange", "()Ljava/nio/ByteBuffer;");
    java_class->codec_dequeueOutputBufferIndex = (*env)->GetStaticMethodID(env, java_HwDecodeBridge, "dequeueOutputBufferIndex", "(J)Ljava/nio/ByteBuffer;");
    java_class->HwDecodeBridge = (*env)->NewGlobalRef(env, java_HwDecodeBridge);
    (*env)->DeleteLocalRef(env, java_HwDecodeBridge);

    jclass java_SurfaceTextureBridge = (*env)->FindClass(env, "com/trinity/decoder/SurfaceTextureBridge");
    java_class->texture_getSurface = (*env)->GetStaticMethodID(env, java_SurfaceTextureBridge, "getSurface", "(I)Landroid/view/Surface;");
    java_class->texture_updateTexImage = (*env)->GetStaticMethodID(env, java_SurfaceTextureBridge, "updateTexImage", "()V");
    java_class->texture_getTransformMatrix = (*env)->GetStaticMethodID(env, java_SurfaceTextureBridge, "getTransformMatrix",  "()[F");
    java_class->texture_release = (*env)->GetStaticMethodID(env, java_SurfaceTextureBridge, "release", "()V");
    java_class->SurfaceTextureBridge = (*env)->NewGlobalRef(env, java_SurfaceTextureBridge);
    (*env)->DeleteLocalRef(env, java_SurfaceTextureBridge);

    *p_jc = java_class;
}

void jni_free(JavaClass **p_jc, JNIEnv *env){
    JavaClass * jc = *p_jc;
    (*env)->DeleteGlobalRef(env, jc->HwDecodeBridge);
    (*env)->DeleteGlobalRef(env, jc->SurfaceTextureBridge);
    free(jc);
    *p_jc = NULL;
}