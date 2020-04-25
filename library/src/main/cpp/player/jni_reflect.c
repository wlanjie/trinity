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
    java_class->player_onPlayStatusChanged = (*env)->GetMethodID(env, player_class, "onPlayStatusChanged", "(I)V");
    java_class->player_onPlayError = (*env)->GetMethodID(env, player_class, "onPlayError", "(I)V");
    (*env)->DeleteLocalRef(env, player_class);
    jclass media_codec_class = (*env)->FindClass(env, "com/trinity/decoder/MediaCodecDecode");
    jmethodID media_codec_construct_id_ = (*env)->GetMethodID(env, media_codec_class, "<init>", "()V");
    jobject media_codec_object = (*env)->NewObject(env, media_codec_class, media_codec_construct_id_);
    java_class->media_codec_object = (*env)->NewGlobalRef(env, media_codec_object);
    java_class->codec_init = (*env)->GetMethodID(env, media_codec_class, "start", "(ILjava/lang/String;IILjava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I");
    java_class->codec_stop = (*env)->GetMethodID(env, media_codec_class, "stop", "()V");
    java_class->codec_flush = (*env)->GetMethodID(env, media_codec_class, "flush",  "()V");
    java_class->codec_dequeueInputBuffer = (*env)->GetMethodID(env, media_codec_class, "dequeueInputBuffer", "(J)I");
    java_class->codec_queueInputBuffer = (*env)->GetMethodID(env, media_codec_class, "queueInputBuffer", "(IIJI)V");
    java_class->codec_getInputBuffer = (*env)->GetMethodID(env, media_codec_class, "getInputBuffer", "(I)Ljava/nio/ByteBuffer;");
    java_class->codec_getOutputBuffer = (*env)->GetMethodID(env, media_codec_class, "getOutputBuffer", "(I)Ljava/nio/ByteBuffer;");
    java_class->codec_releaseOutPutBuffer = (*env)->GetMethodID(env, media_codec_class, "releaseOutputBuffer",  "(I)V");
    java_class->codec_formatChange = (*env)->GetMethodID(env, media_codec_class, "formatChange", "()Ljava/nio/ByteBuffer;");
    java_class->codec_dequeueOutputBufferIndex = (*env)->GetMethodID(env, media_codec_class, "dequeueOutputBufferIndex", "(J)Ljava/nio/ByteBuffer;");
    java_class->texture_updateTexImage = (*env)->GetMethodID(env, media_codec_class, "updateTexImage", "()V");
    java_class->texture_getTransformMatrix = (*env)->GetMethodID(env, media_codec_class, "getTransformMatrix",  "()[F");
    java_class->texture_frameAvailable = (*env)->GetMethodID(env, media_codec_class, "frameAvailable", "()Z");
    (*env)->DeleteLocalRef(env, media_codec_object);
    (*env)->DeleteLocalRef(env, media_codec_class);
    *p_jc = java_class;
}

void jni_free(JavaClass **p_jc, JNIEnv *env){
    JavaClass* jc = *p_jc;
    (*env)->DeleteGlobalRef(env, jc->media_codec_object);
    free(jc);
    *p_jc = NULL;
}