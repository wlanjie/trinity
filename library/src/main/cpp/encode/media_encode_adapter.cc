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
const int MAX_ENCODER_Q_SIZE = 6;
}

MediaEncodeAdapter::MediaEncodeAdapter(JavaVM *vm, jobject object)
    : encoding_(false)
    , sps_write_flag_(false)
    , vm_(vm)
    , object_(nullptr)
    , core_(nullptr)
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
    encoding_ = true;
}

MediaEncodeAdapter::~MediaEncodeAdapter() {
    JNIEnv* env = nullptr;
    vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (nullptr != env) {
        env->DeleteGlobalRef(object_);
        object_ = nullptr;
    }
}

int MediaEncodeAdapter::CreateEncoder(EGLCore *core) {
    LOGI("enter: %s", __func__);
    core_ = new EGLCore();
    core_->Init(core->GetContext());
    bool need_attach = false;
    JNIEnv* env;
    int status = vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (status < 0) {
        if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("AttachCurrentThread failed.");
            return -1;
        }
        need_attach = true;
    }
    int ret = CreateMediaEncoder(env);
    jbyteArray buffer = env->NewByteArray(video_width_ * video_height_ * 3 / 2);
    output_buffer_ = reinterpret_cast<jbyteArray>(env->NewGlobalRef(buffer));
    env->DeleteLocalRef(buffer);
    if (need_attach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("DetachCurrentThread failed.");
            return -1;
        }
    }
    start_time_ = 0;
    fps_change_time_ = -1;
    sps_write_flag_ = true;
    LOGI("leave: %s", __func__);
    return ret;
}

void MediaEncodeAdapter::DestroyEncoder() {
    LOGE("before DestroyEncoder");
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
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
    if (nullptr != core_) {
        core_->Release();
        delete core_;
        core_ = nullptr;
    }
    LOGE("after DestroyEncoder");
}

void MediaEncodeAdapter::Encode(int64_t time, int texture_id) {
    int expectedFrameCount = static_cast<int>(time / 1000.0F * frame_rate_ + 0.5F);
    if (expectedFrameCount < encode_frame_count_) {
        LOGE("drop frame encode_count: %d frame_count: %d", encode_frame_count_, expectedFrameCount);
        return;
    }
    if (start_time_ == 0) {
        start_time_ = getCurrentTime();
    }

    if (fps_change_time_ == -1) {
        fps_change_time_ = getCurrentTime();
    }

//    int64_t current_time = static_cast<int64_t>((getCurrentTime() - start_time_) * speed);
    // need drop frames
    encode_frame_count_++;
    if (EGL_NO_SURFACE != encoder_surface_) {
        core_->MakeCurrent(encoder_surface_);
        render_->ProcessImage(texture_id);
        core_->SetPresentationTime(encoder_surface_, ((khronos_stime_nanoseconds_t) time) * 1000000);
        if (!core_->SwapBuffers(encoder_surface_)) {
            LOGE("eglSwapBuffers(encoder_surface_) returned error %d", eglGetError());
        }
        DrainEncodeData();
    }
}

int MediaEncodeAdapter::DrainEncodeData() {
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return -1;
        }
        needAttach = true;
    }
    jclass clazz = env->GetObjectClass(object_);
    jmethodID drainEncoderFunc = env->GetMethodID(clazz, "drainEncoderFromNative", "([B)I");
    auto bufferSize = env->CallIntMethod(object_, drainEncoderFunc, output_buffer_);
    auto *outputData = reinterpret_cast<uint8_t *>(env->GetByteArrayElements(output_buffer_, 0));
    int size = static_cast<int>(bufferSize);
    jmethodID getLastPresentationTimeUsFunc = env->GetMethodID(clazz, "getLastPresentationTimeUsFromNative", "()J");
    auto lastPresentationTimeUs = (long long) env->CallLongMethod(object_, getLastPresentationTimeUsFunc);
    int timeMills = static_cast<int>(lastPresentationTimeUs / 1000.0f);
    if (size > 0) {
        // push to queue_
        int nalu_type = (outputData[4] & 0x1F);
        if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET == nalu_type) {
            auto *units = new std::vector<NALUnit *>();
            parseH264SpsPps(outputData, size, units);
            auto unitSize = units->size();
            if (unitSize > 2) {
                // 证明是sps和pps后边有I帧
                const char bytesHeader[] = "\x00\x00\x00\x01";
                // string literals have implicit trailing '\0'
                size_t headerLength = 4;
                NALUnit *idrUnit = units->at(2);
                auto idrSize = idrUnit->naluSize + headerLength;
                auto *videoPacket = new VideoPacket();
                videoPacket->buffer = new uint8_t[idrSize];
                memcpy(videoPacket->buffer, bytesHeader, headerLength);
                memcpy(videoPacket->buffer + headerLength, idrUnit->naluBody,
                       static_cast<size_t>(idrUnit->naluSize));
                videoPacket->size = static_cast<int>(idrSize);
                videoPacket->timeMills = timeMills;
                if (videoPacket->size > 0) {
                    packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
                }
            }
            if (sps_write_flag_) {
                auto *videoPacket = new VideoPacket();
                videoPacket->buffer = new uint8_t[size];
                memcpy(videoPacket->buffer, outputData, size);
                videoPacket->size = size;
                videoPacket->timeMills = timeMills;
                if (videoPacket->size > 0) {
                    packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
                }
                sps_write_flag_ = false;
            }
        } else if (size > 0) {
            // 为了兼容有一些设备的MediaCodec编码出来的每一帧有多个Slice的问题(华为荣耀6，华为P9)
            int frameBufferSize = 0;
            size_t headerLength = 4;
            uint8_t *frameBuffer;
            const char bytesHeader[] = "\x00\x00\x00\x01";

            auto *units = new std::vector<NALUnit *>();

            parseH264SpsPps(outputData, size, units);
            std::vector<NALUnit *>::iterator i;
            for (i = units->begin(); i != units->end(); ++i) {
                NALUnit *unit = *i;
                int frameLen = unit->naluSize;
                frameBufferSize += headerLength;
                frameBufferSize += frameLen;
            }
            frameBuffer = new uint8_t[frameBufferSize];
            int frameBufferCursor = 0;
            for (i = units->begin(); i != units->end(); ++i) {
                NALUnit *unit = *i;
                uint8_t *nonIDRFrame = unit->naluBody;
                int nonIDRFrameLen = unit->naluSize;
                memcpy(frameBuffer + frameBufferCursor, bytesHeader, headerLength);
                frameBufferCursor += headerLength;
                memcpy(frameBuffer + frameBufferCursor, nonIDRFrame, nonIDRFrameLen);
                frameBufferCursor += nonIDRFrameLen;
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength] =
                        ((nonIDRFrameLen) >> 24) & 0x00ff;
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 1] =
                        ((nonIDRFrameLen) >> 16) & 0x00ff;
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 2] =
                        ((nonIDRFrameLen) >> 8) & 0x00ff;
                frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 3] =
                        ((nonIDRFrameLen)) & 0x00ff;
                delete unit;
            }
            delete units;

            auto *videoPacket = new VideoPacket();
            videoPacket->buffer = frameBuffer;
            videoPacket->size = frameBufferSize;
            videoPacket->timeMills = timeMills;
            if (videoPacket->size > 0) {
                packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
            }
        }
    }
    env->ReleaseByteArrayElements(output_buffer_, reinterpret_cast<jbyte*>(outputData), 0);
    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }

    if (start_encode_time_ == 0) {
        start_encode_time_ = getCurrentTimeSeconds();
    }
    return size;
}

int MediaEncodeAdapter::CreateMediaEncoder(JNIEnv *env) {
    LOGI("enter: %s", __func__);
    jclass clazz = env->GetObjectClass(object_);
    jmethodID createMediaCodecSurfaceEncoderFunc = env->GetMethodID(clazz,
                                                                    "createMediaCodecSurfaceEncoderFromNative",
                                                                    "(IIII)I");
    int ret = env->CallIntMethod(object_, createMediaCodecSurfaceEncoderFunc, video_width_, video_height_,
                        video_bit_rate_, frame_rate_);
    jmethodID getEncodeSurfaceFromNativeFunc = env->GetMethodID(clazz,
                                                                "getEncodeSurfaceFromNative",
                                                                "()Landroid/view/Surface;");
    jobject surface = env->CallObjectMethod(object_, getEncodeSurfaceFromNativeFunc);
    // 2 create window surface
    encoder_window_ = ANativeWindow_fromSurface(env, surface);
    encoder_surface_ = core_->CreateWindowSurface(encoder_window_);
    auto result = core_->MakeCurrent(encoder_surface_);
    if (!result) {
        LOGE("%s MakeCurrent error", __func__);
    }
    render_ = new OpenGL(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    render_->SetOutput(video_width_, video_height_);
    LOGI("leave: %s", __func__);
    return ret;
}

void MediaEncodeAdapter::DestroyMediaEncoder(JNIEnv *env) {
    jclass clazz = env->GetObjectClass(object_);
    jmethodID signal_end_of_stream_method_id = env->GetMethodID(clazz, "signalEndOfInputStream", "()V");
    env->CallVoidMethod(object_, signal_end_of_stream_method_id);
    int size = DrainEncodeData();
    while (size > 0) {
        size = DrainEncodeData();
    }
    jmethodID closeMediaCodecFunc = env->GetMethodID(clazz, "closeMediaCodecCalledFromNative", "()V");
    env->CallVoidMethod(object_, closeMediaCodecFunc);

    // 2 Release surface
    if (EGL_NO_SURFACE != encoder_surface_) {
        core_->ReleaseSurface(encoder_surface_);
        encoder_surface_ = EGL_NO_SURFACE;
    }

    // Release window
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
