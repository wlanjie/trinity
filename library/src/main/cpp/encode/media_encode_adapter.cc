/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by wlanjie on 2019/4/16.
//

#include <vector>
#include <android/native_window_jni.h>
#include "media_encode_adapter.h"
#include "android_xlog.h"
#include "tools.h"
#include "h264_util.h"
#include "gl.h"

namespace trinity {

namespace {
// See MAX_ENCODER_Q_SIZE in androidmediaencoder.cc.
const int MAX_ENCODER_Q_SIZE = 3;
}

MediaEncodeAdapter::MediaEncodeAdapter(JavaVM *vm, jobject object)
    : encoding_(false)
    , sps_write_flag_(false)
    , vm_(vm)
    , object_(nullptr)
    , core_(nullptr)
    , queue_()
    , encoder_thread_()
    , encoder_surface_(EGL_NO_SURFACE)
    , encoder_window_(nullptr)
    , output_buffer_(nullptr)
    , start_encode_time_(0)
    , render_(nullptr) {
    JNIEnv* env = nullptr;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (nullptr != env) {
        object_ = env->NewGlobalRef(object);
    }
    queue_ = new MessageQueue("MediaEncode message queue");
    InitMessageQueue(queue_);
}

MediaEncodeAdapter::~MediaEncodeAdapter() {
    PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(encoder_thread_, nullptr);
    if (nullptr != queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = nullptr;
    }

    JNIEnv* env = nullptr;
    vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (nullptr != env) {
        env->DeleteGlobalRef(object_);
        object_ = nullptr;
    }
}

void* MediaEncodeAdapter::MessageQueueThread(void *args) {
    auto* adapter = static_cast<MediaEncodeAdapter *>(args);
    adapter->EncodeLoop();
    pthread_exit(nullptr);
}

void MediaEncodeAdapter::HandleMessage(trinity::Message *msg) {
    int what = msg->GetWhat();
    switch (what) {
        case FRAME_AVAILABLE:
            DrainEncodeData();
            break;
        default:
            break;
    }
}

void MediaEncodeAdapter::CreateEncoder(EGLCore *core, int texture_id) {
    core_ = core;
    texture_id_ = texture_id;
    bool need_attach = false;
    JNIEnv* env;
    int status = vm_->GetEnv((void **) (&env), JNI_VERSION_1_6);
    if (status < 0) {
        if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("AttachCurrentThread failed.");
            return;
        }
        need_attach = true;
    }
    CreateMediaEncoder(env);
    LOGI("encoder width: %d height: %d", video_width_, video_height_);
    jbyteArray buffer = env->NewByteArray(video_width_ * video_height_ * 3 / 2);
    output_buffer_ = reinterpret_cast<jbyteArray>(env->NewGlobalRef(buffer));
    env->DeleteLocalRef(buffer);
    if (need_attach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("DetachCurrentThread failed.");
            return;
        }
    }
    pthread_create(&encoder_thread_, nullptr, MessageQueueThread, this);
    start_time_ = 0;
    fps_change_time_ = -1;
    encoding_ = true;
    sps_write_flag_ = true;
}

void MediaEncodeAdapter::DestroyEncoder() {
    LOGI("before DestroyEncoder");
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = vm_->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (vm_->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }
    // 2 Release encoder_
    DestroyMediaEncoder(env);
    // 3 Release output memory
    if (nullptr != output_buffer_) {
        env->DeleteGlobalRef(output_buffer_);
    }
    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
    LOGI("after DestroyEncoder");
}

void MediaEncodeAdapter::Encode(int timeMills) {
    if (start_time_ == 0)
        start_time_ = getCurrentTime();

    if (fps_change_time_ == -1){
        fps_change_time_ = getCurrentTime();
    }

    if (GetQueueSize() > MAX_ENCODER_Q_SIZE) {
        LOGE("HWEncoderAdapter:dropped frame_, encoder_ queue_ full");// See webrtc bug 2887.
        return;
    }
    int64_t curTime = getCurrentTime() - start_time_;
    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fps_change_time_) / 1000.0f * frame_rate_ + 0.5f);
    if (expectedFrameCount < encode_frame_count_) {
        LOGE("drop frame encode_count: %d frame_count: %d", encode_frame_count_, expectedFrameCount);
        return;
    }
    encode_frame_count_++;
    if (EGL_NO_SURFACE != encoder_surface_) {
        core_->MakeCurrent(encoder_surface_);
        render_->ProcessImage(texture_id_);
        core_->SetPresentationTime(encoder_surface_, ((khronos_stime_nanoseconds_t) curTime) * 1000000);
        PostMessage(new Message(FRAME_AVAILABLE));
        if (!core_->SwapBuffers(encoder_surface_)) {
            LOGE("eglSwapBuffers(encoder_surface_) returned error %d", eglGetError());
        }
    }
}

void MediaEncodeAdapter::DrainEncodeData() {
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = vm_->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }
    jclass clazz = env->GetObjectClass(object_);
    jmethodID drainEncoderFunc = env->GetMethodID(clazz, "pullH264StreamFromDrainEncoderFromNative", "([B)J");
    long bufferSize = (long) env->CallLongMethod(object_, drainEncoderFunc, output_buffer_);
    auto *outputData = (uint8_t *) env->GetByteArrayElements(output_buffer_, 0);    // Get data
    int size = (int) bufferSize;
    jmethodID getLastPresentationTimeUsFunc = env->GetMethodID(clazz, "getLastPresentationTimeUsFromNative", "()J");
    auto lastPresentationTimeUs = (long long) env->CallLongMethod(object_, getLastPresentationTimeUsFunc);
    int timeMills = (int) (lastPresentationTimeUs / 1000.0f);

    // push to queue_
    int nalu_type = (outputData[4] & 0x1F);
    if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET == nalu_type) {
        auto* units = new vector<NALUnit*>();
        parseH264SpsPps(outputData, size, units);
        int unitSize = units->size();
        if(unitSize > 2) {
            //证明是sps和pps后边有I帧
            const char bytesHeader[] = "\x00\x00\x00\x01";
            size_t headerLength = 4; //string literals have implicit trailing '\0'
            NALUnit* idrUnit = units->at(2);
            int idrSize = idrUnit->naluSize + headerLength;
            auto *videoPacket = new VideoPacket();
            videoPacket->buffer = new uint8_t[idrSize];
            memcpy(videoPacket->buffer, bytesHeader, headerLength);
            memcpy(videoPacket->buffer + headerLength, idrUnit->naluBody, idrUnit->naluSize);
            videoPacket->size = idrSize;
            videoPacket->timeMills = timeMills;
            if (videoPacket->size > 0)
                packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
        }
        if (sps_write_flag_) {
            auto *videoPacket = new VideoPacket();
            videoPacket->buffer = new uint8_t[size];
            memcpy(videoPacket->buffer, outputData, size);
            videoPacket->size = size;
            videoPacket->timeMills = timeMills;
            if (videoPacket->size > 0)
                packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
            sps_write_flag_ = false;
        }
    } else  if (size > 0){
        //为了兼容有一些设备的MediaCodec编码出来的每一帧有多个Slice的问题(华为荣耀6，华为P9)
        int frameBufferSize = 0;
        size_t headerLength = 4;
        uint8_t* frameBuffer;
        const char bytesHeader[] = "\x00\x00\x00\x01";

        auto* units = new vector<NALUnit*>();

        parseH264SpsPps(outputData, size, units);
        vector<NALUnit*>::iterator i;
        for (i = units->begin(); i != units->end(); ++i) {
            NALUnit* unit = *i;
            int frameLen = unit->naluSize;
            frameBufferSize+=headerLength;
            frameBufferSize+=frameLen;
        }
        frameBuffer = new uint8_t[frameBufferSize];
        int frameBufferCursor = 0;
        for (i = units->begin(); i != units->end(); ++i) {
            NALUnit* unit = *i;
            uint8_t* nonIDRFrame = unit->naluBody;
            int nonIDRFrameLen = unit->naluSize;
            memcpy(frameBuffer + frameBufferCursor, bytesHeader, headerLength);
            frameBufferCursor+=headerLength;
            memcpy(frameBuffer + frameBufferCursor, nonIDRFrame, nonIDRFrameLen);
            frameBufferCursor+=nonIDRFrameLen;
            frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength] = ((nonIDRFrameLen) >> 24) & 0x00ff;
            frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 1] = ((nonIDRFrameLen) >> 16) & 0x00ff;
            frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 2] = ((nonIDRFrameLen) >> 8) & 0x00ff;
            frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 3] = ((nonIDRFrameLen)) & 0x00ff;
            delete unit;
        }
        delete units;

        auto *videoPacket = new VideoPacket();
        videoPacket->buffer = frameBuffer;
        videoPacket->size = frameBufferSize;
        videoPacket->timeMills = timeMills;
        if (videoPacket->size > 0)
            packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
    }
    env->ReleaseByteArrayElements(output_buffer_, (jbyte *) outputData, 0);
    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }

    if (start_encode_time_ == 0){
        start_encode_time_ = getCurrentTimeSeconds();
    }
}

void MediaEncodeAdapter::EncodeLoop() {
    while (encoding_) {
        Message *msg = nullptr;
        if (queue_->DequeueMessage(&msg, true) > 0) {
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                encoding_ = false;
            }
            delete msg;
        }
    }
    LOGI("HWEncoderAdapter Encode Thread ending...");
}

void MediaEncodeAdapter::CreateMediaEncoder(JNIEnv *env) {
    jclass clazz = env->GetObjectClass(object_);
    jmethodID createMediaCodecSurfaceEncoderFunc = env->GetMethodID(clazz,
                                                                    "createMediaCodecSurfaceEncoderFromNative",
                                                                    "(IIII)V");
    env->CallVoidMethod(object_, createMediaCodecSurfaceEncoderFunc, video_width_, video_height_,
                        video_bit_rate_, (int) frame_rate_);
    jmethodID getEncodeSurfaceFromNativeFunc = env->GetMethodID(clazz,
                                                                "getEncodeSurfaceFromNative",
                                                                "()Landroid/view/Surface;");
    jobject surface = env->CallObjectMethod(object_, getEncodeSurfaceFromNativeFunc);
    // 2 create window surface
    encoder_window_ = ANativeWindow_fromSurface(env, surface);
    encoder_surface_ = core_->CreateWindowSurface(encoder_window_);
    render_ = new OpenGL(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    render_->SetOutput(video_width_, video_height_);
}

void MediaEncodeAdapter::DestroyMediaEncoder(JNIEnv *env) {
    jclass clazz = env->GetObjectClass(object_);
    jmethodID closeMediaCodecFunc = env->GetMethodID(clazz, "closeMediaCodecCalledFromNative", "()V");
    env->CallVoidMethod(object_, closeMediaCodecFunc);

    // 2 Release surface
    if (EGL_NO_SURFACE != encoder_surface_) {
        core_->ReleaseSurface(encoder_surface_);
        encoder_surface_ = EGL_NO_SURFACE;
    }

    //Release window
    if (nullptr != encoder_window_) {
        ANativeWindow_release(encoder_window_);
        encoder_window_ = nullptr;
    }

    if (nullptr != render_) {
        delete render_;
        render_ = nullptr;
    }
}

}  // namespace trinity
