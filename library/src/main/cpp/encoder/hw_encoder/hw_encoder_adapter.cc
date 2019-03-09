#include "./hw_encoder_adapter.h"
#include "h264_util.h"

#define LOG_TAG "HWEncoderAdapter"

namespace {
    // See MAX_ENCODER_Q_SIZE in androidmediaencoder.cc.
    const int MAX_ENCODER_Q_SIZE = 3;
}

HWEncoderAdapter::HWEncoderAdapter(JavaVM *g_jvm, jobject obj) {
	LOGI("HWEncoderAdapter()");
    output_buffer_ = NULL;
    queue_ = new MessageQueue("HWEncoder message queue_");
    handler_ = new HWEncoderHandler(this, queue_);
    need_reconfig_encoder_ = false;
    this->vm_ = g_jvm;
    this->obj_ = obj;

    encoder_window_ = NULL;
}

HWEncoderAdapter::~HWEncoderAdapter() {
}

void HWEncoderAdapter::CreateHWEncoder(JNIEnv *env) {
    jclass jcls = env->GetObjectClass(obj_);
    jmethodID createMediaCodecSurfaceEncoderFunc = env->GetMethodID(jcls,
                                                                    "createMediaCodecSurfaceEncoderFromNative",
                                                                    "(IIII)V");
    env->CallVoidMethod(obj_, createMediaCodecSurfaceEncoderFunc, video_width_, video_height_,
                        video_bit_rate_, (int) frameRate);
    jmethodID getEncodeSurfaceFromNativeFunc = env->GetMethodID(jcls,
                                                                "getEncodeSurfaceFromNative",
                                                                "()Landroid/view/Surface;");
    jobject surface = env->CallObjectMethod(obj_,
                                            getEncodeSurfaceFromNativeFunc);
    // 2 create window surface
    encoder_window_ = ANativeWindow_fromSurface(env, surface);
    encoder_surface_ = egl_core_->CreateWindowSurface(encoder_window_);
    renderer_ = new VideoGLSurfaceRender();
    renderer_->Init(video_width_, video_height_);
}

void HWEncoderAdapter::DestroyHWEncoder(JNIEnv *env) {
    jclass jcls = env->GetObjectClass(obj_);
    jmethodID closeMediaCodecFunc = env->GetMethodID(jcls, "closeMediaCodecCalledFromNative",
                                                     "()V");
    env->CallVoidMethod(obj_, closeMediaCodecFunc);

    // 2 Release surface
    if (EGL_NO_SURFACE != encoder_surface_) {
        egl_core_->ReleaseSurface(encoder_surface_);
        encoder_surface_ = EGL_NO_SURFACE;
    }

    //Release window
	if (NULL != encoder_window_) {
		ANativeWindow_release(encoder_window_);
		encoder_window_ = NULL;
	}

    if (renderer_) {
        LOGI("delete renderer_..");
        renderer_->Destroy();
        delete renderer_;
        renderer_ = NULL;
    }
}

void HWEncoderAdapter::CreateEncoder(EGLCore *eglCore, int inputTexId) {
    this->egl_core_ = eglCore;
    this->texture_id_ = inputTexId;
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

    // 1 create encoder_
    this->CreateHWEncoder(env);
    // 2 allocate output memory
    LOGI("width is %d %d", video_width_, video_height_);
    jbyteArray tempOutputBuffer = env->NewByteArray(
            video_width_ * video_height_ * 3 / 2);   // big enough
    output_buffer_ = static_cast<jbyteArray>(env->NewGlobalRef(tempOutputBuffer));
    env->DeleteLocalRef(tempOutputBuffer);

    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
    // 3 Starting Encode Thread
    pthread_create(&encoder_thread_, 0, EncoderThreadCallback, this);

    start_time_ = -1;
    fps_change_time_ = -1;

    encoding_ = true;
    sps_unwrite_flag_ = true;
}

void *HWEncoderAdapter::EncoderThreadCallback(void *myself) {
    HWEncoderAdapter *encoder = (HWEncoderAdapter *) myself;
    encoder->EncodeLoop();
    pthread_exit(0);
    return 0;
}

void HWEncoderAdapter::ReConfigureHWEncoder() {
    //调用Java端使用最新的码率重新配置MediaCodec
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
//	jclass jcls = env->GetObjectClass(obj_);
//	jmethodID reConfigureFunc = env->GetMethodID(jcls, "reConfigureFromNative", "(I)V");
//	env->CallVoidMethod(obj_, reConfigureFunc, video_bit_rate_);
    this->DestroyHWEncoder(env);
    this->CreateHWEncoder(env);
    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
}

void HWEncoderAdapter::ReConfigure(int maxBitRate, int avgBitRate, int fps) {
    this->video_bit_rate_ = maxBitRate;
    need_reconfig_encoder_ = true;
}

void HWEncoderAdapter::HotConfigHWEncoder() {
    LOGI("enter HotConfigHWEncoder");
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
    jclass jcls = env->GetObjectClass(obj_);
    jmethodID hotConfigMediaCodecFunc = env->GetMethodID(jcls,
                                                         "hotConfigEncoderFromNative",
                                                         "(IIII)V");
    env->CallVoidMethod(obj_, hotConfigMediaCodecFunc, video_width_, video_height_, video_bit_rate_,
                        (int) frameRate);

    if (EGL_NO_SURFACE != encoder_surface_) {
        egl_core_->ReleaseSurface(encoder_surface_);
        encoder_surface_ = EGL_NO_SURFACE;
    }

    if (NULL != encoder_window_) {
		ANativeWindow_release(encoder_window_);
		encoder_window_ = NULL;
	}
    jmethodID getEncodeSurfaceFromNativeFunc = env->GetMethodID(jcls, "getEncodeSurfaceFromNative",
                                                                "()Landroid/view/Surface;");
    jobject surface = env->CallObjectMethod(obj_, getEncodeSurfaceFromNativeFunc);
    // 2 create window surface
    encoder_window_ = ANativeWindow_fromSurface(env, surface);
    encoder_surface_ = egl_core_->CreateWindowSurface(encoder_window_);

    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }

    LOGI("leave HotConfigHWEncoder");
}


void HWEncoderAdapter::HotConfig(int maxBitrate, int avgBitrate, int fps) {
    this->video_bit_rate_ = maxBitrate;
    need_hot_config_encoder_ = true;
    ResetFpsStartTimeIfNeed(fps);
}

void HWEncoderAdapter::DestroyEncoder() {
    // 1 Stop Encoder Thread
    handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(encoder_thread_, 0);
    if (queue_) {
        queue_->Abort();
        delete queue_;
        queue_ = NULL;
    }
    delete handler_;
    handler_ = NULL;
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
    this->DestroyHWEncoder(env);
    // 3 Release output memory
    if (output_buffer_)
        env->DeleteGlobalRef(output_buffer_);
    if (needAttach) {
        if (vm_->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
    LOGI("after DestroyEncoder");
}

void HWEncoderAdapter::Encode() {
    if (start_time_ == -1)
        start_time_ = getCurrentTime();

    if (fps_change_time_ == -1){
        fps_change_time_ = getCurrentTime();
    }

    if (handler_->GetQueueSize() > MAX_ENCODER_Q_SIZE) {
        LOGE("HWEncoderAdapter:dropped frame_, encoder_ queue_ full");// See webrtc bug 2887.
        return;
    }
    int64_t curTime = getCurrentTime() - start_time_;
    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fps_change_time_) / 1000.0f * frameRate + 0.5f);
    if (expectedFrameCount < encoded_frame_count_) {
//		LOGI("expectedFrameCount is %d while encoded_frame_count_ is %d", expectedFrameCount, encoded_frame_count_);
        return;
    }
    encoded_frame_count_++;
    if (EGL_NO_SURFACE != encoder_surface_) {
        if (need_reconfig_encoder_) {
            ReConfigureHWEncoder();
            need_reconfig_encoder_ = false;
        }
        if (need_hot_config_encoder_) {
            HotConfigHWEncoder();
            need_hot_config_encoder_ = false;
        }
        egl_core_->MakeCurrent(encoder_surface_);
        renderer_->RenderToView(texture_id_, video_width_, video_height_);
        egl_core_->SetPresentationTime(encoder_surface_,
                                     ((khronos_stime_nanoseconds_t) curTime) * 1000000);
        handler_->PostMessage(new Message(FRAME_AVAILIBLE));
        if (!egl_core_->SwapBuffers(encoder_surface_)) {
            LOGE("eglSwapBuffers(encoder_surface_) returned error %d", eglGetError());
        }
    }
}

void HWEncoderAdapter::EncodeLoop() {
    while (encoding_) {
        Message *msg = NULL;
        if (queue_->DequeueMessage(&msg, true) > 0) {
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                encoding_ = false;
            }
            delete msg;
        }
    }
    LOGI("HWEncoderAdapter Encode Thread ending...");
}

void HWEncoderAdapter::DrainEncodedData() {
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
    jclass jcls = env->GetObjectClass(obj_);
    jmethodID drainEncoderFunc = env->GetMethodID(jcls, "pullH264StreamFromDrainEncoderFromNative",
                                                  "([B)J");
    long bufferSize = (long) env->CallLongMethod(obj_, drainEncoderFunc, output_buffer_);
    byte *outputData = (uint8_t *) env->GetByteArrayElements(output_buffer_, 0);    // Get data
    int size = (int) bufferSize;
//	LOGI("Size is %d", Size);

    jmethodID getLastPresentationTimeUsFunc = env->GetMethodID(jcls,
                                                               "getLastPresentationTimeUsFromNative",
                                                               "()J");
    long long lastPresentationTimeUs = (long long) env->CallLongMethod(obj_,
                                                                       getLastPresentationTimeUsFunc);

    int timeMills = (int) (lastPresentationTimeUs / 1000.0f);

    // push to queue_
    int nalu_type = (outputData[4] & 0x1F);
    LOGI("nalu_type is %d... Size is %d", nalu_type, size);
    if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET == nalu_type) {
        vector<NALUnit*>* units = new vector<NALUnit*>();
        parseH264SpsPps(outputData, size, units);
        int unitSize = units->size();
        LOGI("unitSize is %d", unitSize);
        if(unitSize > 2) {
            //证明是sps和pps后边有I帧
            const char bytesHeader[] = "\x00\x00\x00\x01";
            size_t headerLength = 4; //string literals have implicit trailing '\0'
            NALUnit* idrUnit = units->at(2);
            int idrSize = idrUnit->naluSize + headerLength;
            LiveVideoPacket *videoPacket = new LiveVideoPacket();
            videoPacket->buffer = new byte[idrSize];
            memcpy(videoPacket->buffer, bytesHeader, headerLength);
            memcpy(videoPacket->buffer + headerLength, idrUnit->naluBody, idrUnit->naluSize);
            videoPacket->size = idrSize;
            videoPacket->timeMills = timeMills;
            if (videoPacket->size > 0)
                packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
        }
        if (sps_unwrite_flag_) {
            LiveVideoPacket *videoPacket = new LiveVideoPacket();
            videoPacket->buffer = new byte[size];
            memcpy(videoPacket->buffer, outputData, size);
            videoPacket->size = size;
            videoPacket->timeMills = timeMills;
            if (videoPacket->size > 0)
                packet_pool_->PushRecordingVideoPacketToQueue(videoPacket);
            sps_unwrite_flag_ = false;
        }
    } else  if (size > 0){
        //为了兼容有一些设备的MediaCodec编码出来的每一帧有多个Slice的问题(华为荣耀6，华为P9)
        int frameBufferSize = 0;
        size_t headerLength = 4;
        byte* frameBuffer;
        const char bytesHeader[] = "\x00\x00\x00\x01";

        vector<NALUnit*>* units = new vector<NALUnit*>();

        parseH264SpsPps(outputData, size, units);
        vector<NALUnit*>::iterator i;
        for (i = units->begin(); i != units->end(); ++i) {
            NALUnit* unit = *i;
            int frameLen = unit->naluSize;
            frameBufferSize+=headerLength;
            frameBufferSize+=frameLen;
        }
        frameBuffer = new byte[frameBufferSize];
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

        LiveVideoPacket *videoPacket = new LiveVideoPacket();
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
        start_encode_time_ = platform_4_live::getCurrentTimeSeconds();
    }
}
