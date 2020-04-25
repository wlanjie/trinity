//
//  media_codec.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <unistd.h>
#include <pthread.h>
#include "media_codec.h"

#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION

int mediacodec_send_packet(AVPlayContext *context, AVPacket *packet, int buffer_index) {
    MediaCodecContext *media_codec_context = context->media_codec_context;
    if (packet == NULL) {
        return -2;
    }
    uint32_t keyframe_flag = 0;
//    av_packet_split_side_data(packet);
    int64_t time_stamp = packet->pts;
    if (!time_stamp && packet->dts)
        time_stamp = packet->dts;
    if (time_stamp > 0) {
        time_stamp = av_rescale_q(time_stamp, context->format_context->streams[context->video_index]->time_base,
                                  AV_TIME_BASE_Q);
    } else {
        time_stamp = 0;
    }
    if (media_codec_context->codec_id == AV_CODEC_ID_H264 ||
        media_codec_context->codec_id == AV_CODEC_ID_HEVC) {
        H264ConvertState convert_state = {0, 0};
        convert_h264_to_annexb(packet->data, (size_t) packet->size, media_codec_context->nal_size, &convert_state);
    }
    if ((packet->flags | AV_PKT_FLAG_KEY) > 0) {
        keyframe_flag |= 0x1;
    }
    media_status_t media_status;
    size_t size;
    if (buffer_index >= 0) {
        uint8_t *buf = AMediaCodec_getInputBuffer(media_codec_context->codec, (size_t) buffer_index, &size);
        if (buf != NULL && size >= packet->size) {
            memcpy(buf, packet->data, (size_t) packet->size);
            media_status = AMediaCodec_queueInputBuffer(media_codec_context->codec, (size_t) buffer_index, 0, (size_t) packet->size,
                                                        (uint64_t) time_stamp,
                                                        keyframe_flag);
            if (media_status != AMEDIA_OK) {
                LOGE("AMediaCodec_queueInputBuffer error. status ==> %d", media_status);
                return (int) media_status;
            }
        }
    }else if(buffer_index == AMEDIACODEC_INFO_TRY_AGAIN_LATER){
        return -1;
    }else{
        LOGE("input buffer id < 0  value == %zd", buffer_index);
    }
    return 0;
}

int mediacodec_dequeue_input_buffer_index(AVPlayContext* context) {
    if (context == NULL) {
        return -1;
    }
    MediaCodecContext *media_codec_context = context->media_codec_context;
    ssize_t id = AMediaCodec_dequeueInputBuffer(media_codec_context->codec, 1000000);
    return (int) id;
}

void mediacodec_release_buffer(AVPlayContext *pd, AVFrame *frame) {
    AMediaCodec_releaseOutputBuffer(pd->media_codec_context->codec, (size_t) frame->HW_BUFFER_ID, true);
}

int mediacodec_receive_frame(AVPlayContext *pd, AVFrame *frame) {
    MediaCodecContext *ctx = pd->media_codec_context;
    AMediaCodecBufferInfo info;
    int output_ret = 1;
    ssize_t outbufidx = AMediaCodec_dequeueOutputBuffer(ctx->codec, &info, 0);
    if (outbufidx >= 0) {
            frame->pts = info.presentationTimeUs;
            frame->format = PIX_FMT_EGL_EXT;
            frame->width = pd->width;
            frame->linesize[0] = pd->width;
            frame->height = pd->height;
            frame->HW_BUFFER_ID = outbufidx;
            output_ret = 0;
    } else {
        switch (outbufidx) {
            case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED: {
                int pix_format = -1;
                AMediaFormat *format = AMediaCodec_getOutputFormat(ctx->codec);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &ctx->width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &ctx->height);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &pix_format);
                //todo 仅支持了两种格式
                switch (pix_format) {
                    case 19:
                        ctx->pix_format = AV_PIX_FMT_YUV420P;
                        break;
                    case 21:
                        ctx->pix_format = AV_PIX_FMT_NV12;
                        break;
                    default:
                        break;
                }
                output_ret = -2;
                break;
            }
            case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
                break;
            case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
                break;
            default:
                break;
        }

    }
    return output_ret;
}

void mediacodec_flush(AVPlayContext *pd) {
    MediaCodecContext *ctx = pd->media_codec_context;
    AMediaCodec_flush(ctx->codec);
}

MediaCodecContext *create_mediacodec_context(AVPlayContext *pd) {
    MediaCodecContext *ctx = (MediaCodecContext *) malloc(sizeof(MediaCodecContext));
    AVCodecParameters *codecpar = pd->format_context->streams[pd->video_index]->codecpar;
    ctx->width = codecpar->width;
    ctx->height = codecpar->height;
    ctx->codec_id = codecpar->codec_id;
    ctx->nal_size = 0;
    ctx->format = AMediaFormat_new();
    ctx->pix_format = AV_PIX_FMT_NONE;
//    "video/x-vnd.on2.vp8" - VP8 video (i.e. video in .webm)
//    "video/x-vnd.on2.vp9" - VP9 video (i.e. video in .webm)
//    "video/avc" - H.264/AVC video
//    "video/hevc" - H.265/HEVC video
//    "video/mp4v-es" - MPEG4 video
//    "video/3gpp" - H.263 video
    switch (ctx->codec_id) {
        case AV_CODEC_ID_H264:
            ctx->codec = AMediaCodec_createDecoderByType("video/avc");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/avc");
            if (codecpar->extradata[0] == 1) {
                size_t sps_size, pps_size;
                uint8_t *sps_buf;
                uint8_t *pps_buf;
                sps_buf = (uint8_t *) malloc((size_t) codecpar->extradata_size + 20);
                pps_buf = (uint8_t *) malloc((size_t) codecpar->extradata_size + 20);
                if (0 != convert_sps_pps2(codecpar->extradata, (size_t) codecpar->extradata_size,
                                          sps_buf, &sps_size, pps_buf, &pps_size, &ctx->nal_size)) {
                    LOGE("%s:convert_sps_pps: failed\n", __func__);
                }
                AMediaFormat_setBuffer(ctx->format, "csd-0", sps_buf, sps_size);
                AMediaFormat_setBuffer(ctx->format, "csd-1", pps_buf, pps_size);
                free(sps_buf);
                free(pps_buf);
            }
            break;
        case AV_CODEC_ID_HEVC:
            ctx->codec = AMediaCodec_createDecoderByType("video/hevc");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/hevc");
            if (codecpar->extradata_size > 3 &&
                (codecpar->extradata[0] == 1 || codecpar->extradata[1] == 1)) {
                size_t sps_pps_size = 0;
                size_t convert_size = (size_t) (codecpar->extradata_size + 20);
                uint8_t *convert_buf = (uint8_t *) malloc((size_t) convert_size);
                if (0 != convert_hevc_nal_units(codecpar->extradata, (size_t) codecpar->extradata_size,
                                                convert_buf, convert_size, &sps_pps_size,
                                                &ctx->nal_size)) {
                    LOGE("%s:convert_sps_pps: failed\n", __func__);
                }
                AMediaFormat_setBuffer(ctx->format, "csd-0", convert_buf, sps_pps_size);
                free(convert_buf);
            }
            break;
        case AV_CODEC_ID_MPEG4:
            ctx->codec = AMediaCodec_createDecoderByType("video/mp4v-es");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/mp4v-es");
            AMediaFormat_setBuffer(ctx->format, "csd-0", codecpar->extradata,
                                   (size_t) codecpar->extradata_size);
            break;
        case AV_CODEC_ID_VP8:
            ctx->codec = AMediaCodec_createDecoderByType("video/x-vnd.on2.vp8");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/x-vnd.on2.vp8");
            break;
        case AV_CODEC_ID_VP9:
            ctx->codec = AMediaCodec_createDecoderByType("video/x-vnd.on2.vp9");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/x-vnd.on2.vp9");
            break;
        case AV_CODEC_ID_H263:
            ctx->codec = AMediaCodec_createDecoderByType("video/3gpp");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/3gpp");
            AMediaFormat_setBuffer(ctx->format, "csd-0", codecpar->extradata,
                                   (size_t) codecpar->extradata_size);
            break;
        default:
            break;
    }
//    AMediaFormat_setInt32(ctx->format, "rotation-degrees", 90);
    AMediaFormat_setInt32(ctx->format, AMEDIAFORMAT_KEY_WIDTH, ctx->width);
    AMediaFormat_setInt32(ctx->format, AMEDIAFORMAT_KEY_HEIGHT, ctx->height);
//    media_status_t ret = AMediaCodec_configure(ctx->codec, ctx->format, NULL, NULL, 0);

    return ctx;
}

void mediacodec_start(AVPlayContext *pd){
    MediaCodecContext *ctx = pd->media_codec_context;
    while(pd->window == NULL){
        usleep(10000);
    }
    media_status_t ret = AMediaCodec_configure(ctx->codec, ctx->format, pd->window, NULL, 0);
    if (ret != AMEDIA_OK) {
        LOGE("open mediacodec failed \n");
    }
    ret = AMediaCodec_start(ctx->codec);
    if (ret != AMEDIA_OK) {
        LOGE("open mediacodec failed \n");
    }
}

void mediacodec_stop(AVPlayContext * pd){
    AMediaCodec_stop(pd->media_codec_context->codec);
}

void mediacodec_release_context(AVPlayContext * pd){
    MediaCodecContext *ctx = pd->media_codec_context;
    AMediaCodec_delete(ctx->codec);
    AMediaFormat_delete(ctx->format);
    free(ctx);
    pd->media_codec_context = NULL;
}
#else

static int get_int(uint8_t *buf) {
    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static int64_t get_long(uint8_t *buf) {
    return (((int64_t) buf[0]) << 56)
           + (((int64_t) buf[1]) << 48)
           + (((int64_t) buf[2]) << 40)
           + (((int64_t) buf[3]) << 32)
           + (((int64_t) buf[4]) << 24)
           + (((int64_t) buf[5]) << 16)
           + (((int64_t) buf[6]) << 8)
           + ((int64_t) buf[7]);
}

MediaCodecContext *create_mediacodec_context(
        AVPlayContext *context) {
    LOGI("enter %s", __func__);
    MediaCodecContext *media_codec_context = (MediaCodecContext *) malloc(sizeof(MediaCodecContext));
    AVCodecParameters *codecpar = context->format_context->streams[context->video_index]->codecpar;
    media_codec_context->width = codecpar->width;
    media_codec_context->height = codecpar->height;
    media_codec_context->codec_id = codecpar->codec_id;
    media_codec_context->nal_size = 0;
    media_codec_context->pix_format = AV_PIX_FMT_NONE;
    LOGI("leave %s", __func__);
    return media_codec_context;
}

int mediacodec_start(AVPlayContext *context){
    MediaCodecContext *media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        return -1;
    }
    JNIEnv *env = NULL;
    int status = (*context->vm)->GetEnv(context->vm , (void**)(&env), JNI_VERSION_1_6);
    if (status < 0) {
        (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    }
    JavaClass* java_class = context->java_class;
    AVCodecParameters *codecpar = context->format_context->streams[context->video_index]->codecpar;
    jobject codecName = NULL;
    jobject csd_0 = NULL;
    jobject csd_1 = NULL;
    LOGI("enter %s", __func__);
    int ret = 0;
    switch (media_codec_context->codec_id) {
        case AV_CODEC_ID_H264:
            codecName = (*env)->NewStringUTF(env, "video/avc");
            if (codecpar->extradata[0] == 1) {
                size_t sps_size, pps_size;
                uint8_t *sps_buf;
                uint8_t *pps_buf;
                sps_buf = (uint8_t *) malloc((size_t) codecpar->extradata_size + 20);
                pps_buf = (uint8_t *) malloc((size_t) codecpar->extradata_size + 20);
                if (0 != convert_sps_pps2(codecpar->extradata, (size_t) codecpar->extradata_size,
                                          sps_buf, &sps_size, pps_buf, &pps_size, &media_codec_context->nal_size)) {
                    LOGE("%s:convert_sps_pps: failed\n", __func__);
                }
                csd_0 = (*env)->NewDirectByteBuffer(env, sps_buf, sps_size);
                csd_1 = (*env)->NewDirectByteBuffer(env, pps_buf, pps_size);
                ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                                                context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, csd_0, csd_1);
                free(sps_buf);
                free(pps_buf);
                (*env)->DeleteLocalRef(env, csd_0);
                (*env)->DeleteLocalRef(env, csd_1);
            } else {
                ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                                                context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, NULL, NULL);
            }
            break;
        case AV_CODEC_ID_HEVC:
            codecName = (*env)->NewStringUTF(env, "video/hevc");
            if (codecpar->extradata_size > 3 &&
                (codecpar->extradata[0] == 1 || codecpar->extradata[1] == 1)) {
                size_t sps_pps_size = 0;
                size_t convert_size = (size_t) (codecpar->extradata_size + 20);
                uint8_t *convert_buf = (uint8_t *) malloc((size_t) convert_size);
                if (0 != convert_hevc_nal_units(codecpar->extradata, (size_t) codecpar->extradata_size,
                                           convert_buf, convert_size, &sps_pps_size,
                                           &media_codec_context->nal_size)) {
                    LOGE("%s:convert_sps_pps: failed\n", __func__);
                }
                csd_0 = (*env)->NewDirectByteBuffer(env, convert_buf, sps_pps_size);
                ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                                                context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, csd_0, NULL);
                free(convert_buf);
                (*env)->DeleteLocalRef(env, csd_0);
            }else{
                ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init, codecName,
                                                context->media_codec_texture_id, media_codec_context->width, media_codec_context->height, NULL, NULL);
            }
            break;
        case AV_CODEC_ID_MPEG4:
            codecName = (*env)->NewStringUTF(env, "video/mp4v-es");
            csd_0 = (*env)->NewDirectByteBuffer(env, codecpar->extradata,
                                                   (jlong) (codecpar->extradata_size));
            ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                    context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, csd_0, NULL);
            (*env)->DeleteLocalRef(env, csd_0);
            break;
        case AV_CODEC_ID_VP8:
            codecName = (*env)->NewStringUTF(env, "video/x-vnd.on2.vp8");
            ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                    context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, NULL, NULL);
            break;
        case AV_CODEC_ID_VP9:
            codecName = (*env)->NewStringUTF(env, "video/x-vnd.on2.vp9");
            ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                    context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, NULL, NULL);
            break;
        case AV_CODEC_ID_H263:
            codecName = (*env)->NewStringUTF(env, "video/3gpp");
            csd_0 = (*env)->NewDirectByteBuffer(env, codecpar->extradata,
                                                   (jlong) (codecpar->extradata_size));
            ret = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_init,
                    context->media_codec_texture_id, codecName, media_codec_context->width, media_codec_context->height, csd_0, NULL);
            (*env)->DeleteLocalRef(env, csd_0);
            break;
        default:
            break;
    }
    if (codecName != NULL) {
        (*env)->DeleteLocalRef(env, codecName);
    }
    if (status < 0) {
        (*context->vm)->DetachCurrentThread(context->vm);
    }
    LOGI("leave %s", __func__);
    return ret;
}

void mediacodec_release_buffer(AVPlayContext *context, AVFrame *frame) {
    pthread_mutex_lock(&context->media_codec_mutex);
    if (context->media_codec_context == NULL) {
        LOGE("context->media_codec_context == NULL");
        pthread_mutex_unlock(&context->media_codec_mutex);
        return;
    }
    JavaVM* vm = context->vm;
    JNIEnv* env = NULL;
    if ((*vm)->AttachCurrentThread(vm, &env, NULL) != JNI_OK) {
        pthread_mutex_unlock(&context->media_codec_mutex);
        LOGE("AttachCurrentThread error");
        return;
    }
    JavaClass* java_class = context->java_class;
    (*env)->CallVoidMethod(env, java_class->media_codec_object, java_class->codec_releaseOutPutBuffer,
                                    (int)frame->HW_BUFFER_ID);
    (*vm)->DetachCurrentThread(vm);
    pthread_mutex_unlock(&context->media_codec_mutex);
}

int mediacodec_receive_frame(AVPlayContext *context, AVFrame *frame) {
    MediaCodecContext *media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        return -1;
    }
    JNIEnv *env = NULL;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    JavaClass* java_class = context->java_class;
    int output_ret = -1;
    jobject deqret = (*env)->CallObjectMethod(env, java_class->media_codec_object,
                                                       java_class->codec_dequeueOutputBufferIndex,
                                                       (jlong) 0);
    uint8_t* ret_buf = (*env)->GetDirectBufferAddress(env, deqret);
    int out_index = get_int(ret_buf);
    int flags = get_int(ret_buf + 4);
    int64_t pts = get_long(ret_buf + 8);
    (*env)->DeleteLocalRef(env, deqret);
    if (out_index >= 0) {
        frame->pts = pts;
        frame->format = PIX_FMT_EGL_EXT;
        frame->width = context->width;
        frame->linesize[0] = context->width;
        frame->height = context->height;
        frame->HW_BUFFER_ID = out_index;
        output_ret = flags;
    } else {
        switch (out_index) {
            case -2: {
                jobject newFormat = (*env)->CallObjectMethod(env, java_class->media_codec_object,
                                                                      java_class->codec_formatChange);
                uint8_t *fmtbuf = (*env)->GetDirectBufferAddress(env, newFormat);
                media_codec_context->width = get_int(fmtbuf);
                media_codec_context->height = get_int(fmtbuf + 4);
                int pix_format = get_int(fmtbuf + 8);
                (*env)->DeleteLocalRef(env, newFormat);

                //todo 仅支持了两种格式
                switch (pix_format) {
                    case 19:
                        media_codec_context->pix_format = AV_PIX_FMT_YUV420P;
                        break;
                    case 21:
                        media_codec_context->pix_format = AV_PIX_FMT_NV12;
                        break;
                    default:
                        break;
                }
                output_ret = -2;
                break;
            }
            case -3:
                break;
            case -1:
                break;
            default:
                break;
        }

    }
    (*context->vm)->DetachCurrentThread(context->vm);
    return output_ret;
}

int mediacodec_dequeue_input_buffer_index(AVPlayContext* context) {
    if (context == NULL) {
        return -1;
    }
    JNIEnv *env = NULL;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    JavaClass* java_class = context->java_class;
    int id = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_dequeueInputBuffer, (jlong) 100 * 1000);
    (*context->vm)->DetachCurrentThread(context->vm);
    return id;
}

int mediacodec_send_packet(AVPlayContext *context, AVPacket *packet, int buffer_index) {
    if (packet == NULL) {
        return -2;
    }
    MediaCodecContext *media_codec_context = context->media_codec_context;
    JNIEnv *env = NULL;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    JavaClass* java_class = context->java_class;
    int keyframe_flag = 0;
//    av_packet_split_side_data(packet);
    int64_t time_stamp = packet->pts;
    if (!time_stamp && packet->dts) {
        time_stamp = packet->dts;
    }
    if (time_stamp > 0) {
        time_stamp = av_rescale_q(time_stamp,
                context->format_context->streams[context->video_index]->time_base,
                AV_TIME_BASE_Q);
    } else {
        time_stamp = 0;
    }
    if (media_codec_context->codec_id == AV_CODEC_ID_H264 ||
        media_codec_context->codec_id == AV_CODEC_ID_HEVC) {
        H264ConvertState convert_state = {0, 0};
        convert_h264_to_annexb(packet->data, packet->size, media_codec_context->nal_size, &convert_state);
    }
    if ((packet->flags | AV_PKT_FLAG_KEY) > 0) {
        keyframe_flag |= 0x1;
    }
//    int id = (*env)->CallIntMethod(env, java_class->media_codec_object, java_class->codec_dequeueInputBuffer, (jlong) 100 * 1000);
    if (buffer_index >= 0) {
        jobject inputBuffer = (*env)->CallObjectMethod(env, java_class->media_codec_object,
                java_class->codec_getInputBuffer, buffer_index);
        uint8_t *buf = (*env)->GetDirectBufferAddress(env, inputBuffer);
        jlong size = (*env)->GetDirectBufferCapacity(env, inputBuffer);
        if (buf != NULL && size >= packet->size) {
            memcpy(buf, packet->data, (size_t) packet->size);
            (*env)->CallVoidMethod(env, java_class->media_codec_object,
                                            java_class->codec_queueInputBuffer,
                                            (jint) buffer_index, (jint) packet->size,
                                            (jlong) time_stamp, (jint) keyframe_flag);
        }
        (*env)->DeleteLocalRef(env, inputBuffer);
    } else if (buffer_index == -1) {
        (*context->vm)->DetachCurrentThread(context->vm);
        return -1;
    } else {
        LOGE("input buffer id < 0  value == %zd", buffer_index);
    }

    (*context->vm)->DetachCurrentThread(context->vm);
    return 0;
}

void mediacodec_end_of_stream(AVPlayContext* context, int buffer_index) {
    JNIEnv *env = NULL;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    JavaClass* java_class = context->java_class;
    (*env)->CallVoidMethod(env, java_class->media_codec_object,
            java_class->codec_queueInputBuffer,
            buffer_index, 0, (jlong) 0, BUFFER_FLAG_END_OF_STREAM);
    (*context->vm)->DetachCurrentThread(context->vm);
}

void mediacodec_flush(AVPlayContext *context) {
    LOGI("enter %s", __func__);
    pthread_mutex_lock(&context->media_codec_mutex);
    MediaCodecContext* media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        pthread_mutex_unlock(&context->media_codec_mutex);
        return;
    }
    JNIEnv *env = NULL;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    JavaClass* java_class = context->java_class;
    (*env)->CallVoidMethod(env, java_class->media_codec_object, java_class->codec_flush);
    (*context->vm)->DetachCurrentThread(context->vm);
    LOGI("leave %s", __func__);
    pthread_mutex_unlock(&context->media_codec_mutex);
}

void mediacodec_seek(AVPlayContext* context) {
    MediaCodecContext *media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        return;
    }
    JNIEnv *env = NULL;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    while (1) {
        JavaClass *java_class = context->java_class;
        jobject deqret = (*env)->CallObjectMethod(env, java_class->media_codec_object,
                                                  java_class->codec_dequeueOutputBufferIndex,
                                                  (jlong) 0);
        uint8_t *ret_buf = (*env)->GetDirectBufferAddress(env, deqret);
        int out_index = get_int(ret_buf);
        (*env)->DeleteLocalRef(env, deqret);
        if (out_index >= 0) {
            (*env)->CallVoidMethod(env, java_class->media_codec_object, java_class->codec_releaseOutPutBuffer,
                                   out_index);
        } else {
            break;
        }
    }
    (*context->vm)->DetachCurrentThread(context->vm);
}

void mediacodec_stop(AVPlayContext *context) {
    LOGE("enter %s", __func__);
    pthread_mutex_lock(&context->media_codec_mutex);
    MediaCodecContext* media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        pthread_mutex_unlock(&context->media_codec_mutex);
        return;
    }
    JNIEnv *env = NULL;
    int status = (*context->vm)->GetEnv(context->vm , (void**)(&env), JNI_VERSION_1_6);
    if (status < 0) {
        (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    }
    JavaClass* java_class = context->java_class;
    (*env)->CallVoidMethod(env, java_class->media_codec_object, java_class->codec_stop);
    if (status < 0) {
        (*context->vm)->DetachCurrentThread(context->vm);
    }
    free(context->media_codec_context);
    context->media_codec_context = NULL;
    pthread_mutex_unlock(&context->media_codec_mutex);
    LOGE("leave %s", __func__);
}

void mediacodec_update_image(AVPlayContext* context) {
    pthread_mutex_lock(&context->media_codec_mutex);
    MediaCodecContext* media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        pthread_mutex_unlock(&context->media_codec_mutex);
        return;
    }
    JNIEnv *env = NULL;
    JavaClass* java_class = context->java_class;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    (*env)->CallVoidMethod(env, java_class->media_codec_object, java_class->texture_updateTexImage);
    (*context->vm)->DetachCurrentThread(context->vm);
    pthread_mutex_unlock(&context->media_codec_mutex);
}

int mediacodec_get_texture_matrix(AVPlayContext* context, float* texture_matrix) {
    pthread_mutex_lock(&context->media_codec_mutex);
    MediaCodecContext* media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        pthread_mutex_unlock(&context->media_codec_mutex);
        return -1;
    }
    JNIEnv *env = NULL;
    JavaClass* java_class = context->java_class;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    jfloatArray texture_matrix_array = (*env)->CallObjectMethod(env, java_class->media_codec_object,
            java_class->texture_getTransformMatrix);
    (*env)->GetFloatArrayRegion(env, texture_matrix_array, 0, 16, texture_matrix);
    (*env)->DeleteLocalRef(env, texture_matrix_array);
    (*context->vm)->DetachCurrentThread(context->vm);
    pthread_mutex_unlock(&context->media_codec_mutex);
    return 0;
}

bool mediacodec_frame_available(AVPlayContext* context) {
    pthread_mutex_lock(&context->media_codec_mutex);
    MediaCodecContext* media_codec_context = context->media_codec_context;
    if (media_codec_context == NULL) {
        pthread_mutex_unlock(&context->media_codec_mutex);
        return false;
    }
    JNIEnv *env = NULL;
    JavaClass* java_class = context->java_class;
    (*context->vm)->AttachCurrentThread(context->vm, &env, NULL);
    jboolean frame_available = (*env)->CallBooleanMethod(env,
            java_class->media_codec_object, java_class->texture_frameAvailable);
    (*context->vm)->DetachCurrentThread(context->vm);
    pthread_mutex_unlock(&context->media_codec_mutex);
    return frame_available;
}

#endif