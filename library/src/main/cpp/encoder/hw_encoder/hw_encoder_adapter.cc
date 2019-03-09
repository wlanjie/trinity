#include "./hw_encoder_adapter.h"
#include "h264_util.h"

#define LOG_TAG "HWEncoderAdapter"

namespace {
    // See MAX_ENCODER_Q_SIZE in androidmediaencoder.cc.
    const int MAX_ENCODER_Q_SIZE = 3;
}

HWEncoderAdapter::HWEncoderAdapter(JavaVM *g_jvm, jobject obj) {
	LOGI("HWEncoderAdapter()");
    outputBuffer = NULL;
    queue = new MessageQueue("HWEncoder message queue_");
    handler = new HWEncoderHandler(this, queue);
    isNeedReconfigEncoder = false;
    this->g_jvm = g_jvm;
    this->obj = obj;

    encoderWindow = NULL;
}

HWEncoderAdapter::~HWEncoderAdapter() {
}

void HWEncoderAdapter::createHWEncoder(JNIEnv *env) {
    jclass jcls = env->GetObjectClass(obj);
    jmethodID createMediaCodecSurfaceEncoderFunc = env->GetMethodID(jcls,
                                                                    "createMediaCodecSurfaceEncoderFromNative",
                                                                    "(IIII)V");
    env->CallVoidMethod(obj, createMediaCodecSurfaceEncoderFunc, videoWidth, videoHeight,
                        videoBitRate, (int) frameRate);
    jmethodID getEncodeSurfaceFromNativeFunc = env->GetMethodID(jcls,
                                                                "getEncodeSurfaceFromNative",
                                                                "()Landroid/view/Surface;");
    jobject surface = env->CallObjectMethod(obj,
                                            getEncodeSurfaceFromNativeFunc);
    // 2 create window surface
    encoderWindow = ANativeWindow_fromSurface(env, surface);
    encoderSurface = eglCore->createWindowSurface(encoderWindow);
    renderer = new VideoGLSurfaceRender();
    renderer->init(videoWidth, videoHeight);
}

void HWEncoderAdapter::destroyHWEncoder(JNIEnv *env) {
    jclass jcls = env->GetObjectClass(obj);
    jmethodID closeMediaCodecFunc = env->GetMethodID(jcls, "closeMediaCodecCalledFromNative",
                                                     "()V");
    env->CallVoidMethod(obj, closeMediaCodecFunc);

    // 2 release surface
    if (EGL_NO_SURFACE != encoderSurface) {
        eglCore->releaseSurface(encoderSurface);
        encoderSurface = EGL_NO_SURFACE;
    }

    //release window
	if (NULL != encoderWindow) {
		ANativeWindow_release(encoderWindow);
		encoderWindow = NULL;
	}

    if (renderer) {
        LOGI("delete renderer_..");
        renderer->dealloc();
        delete renderer;
        renderer = NULL;
    }
}

void HWEncoderAdapter::createEncoder(EGLCore *eglCore, int inputTexId) {
    this->eglCore = eglCore;
    this->texId = inputTexId;
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = g_jvm->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }

    // 1 create encoder_
    this->createHWEncoder(env);
    // 2 allocate output memory
    LOGI("width is %d %d", videoWidth, videoHeight);
    jbyteArray tempOutputBuffer = env->NewByteArray(
            videoWidth * videoHeight * 3 / 2);   // big enough
    outputBuffer = static_cast<jbyteArray>(env->NewGlobalRef(tempOutputBuffer));
    env->DeleteLocalRef(tempOutputBuffer);

    if (needAttach) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
    // 3 Starting Encode Thread
    pthread_create(&mEncoderThreadId, 0, encoderThreadCallback, this);

    startTime = -1;
    fpsChangeTime = -1;

    isEncoding = true;
    isSPSUnWriteFlag = true;
}

void *HWEncoderAdapter::encoderThreadCallback(void *myself) {
    HWEncoderAdapter *encoder = (HWEncoderAdapter *) myself;
    encoder->encodeLoop();
    pthread_exit(0);
    return 0;
}

void HWEncoderAdapter::reConfigureHWEncoder() {
    //调用Java端使用最新的码率重新配置MediaCodec
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = g_jvm->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }
//	jclass jcls = env->GetObjectClass(obj_);
//	jmethodID reConfigureFunc = env->GetMethodID(jcls, "reConfigureFromNative", "(I)V");
//	env->CallVoidMethod(obj_, reConfigureFunc, videoBitRate);
    this->destroyHWEncoder(env);
    this->createHWEncoder(env);
    if (needAttach) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
}

void HWEncoderAdapter::reConfigure(int maxBitRate, int avgBitRate, int fps) {
    this->videoBitRate = maxBitRate;
    isNeedReconfigEncoder = true;
}

void HWEncoderAdapter::hotConfigHWEncoder() {
    LOGI("enter hotConfigHWEncoder");
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = g_jvm->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }
    jclass jcls = env->GetObjectClass(obj);
    jmethodID hotConfigMediaCodecFunc = env->GetMethodID(jcls,
                                                         "hotConfigEncoderFromNative",
                                                         "(IIII)V");
    env->CallVoidMethod(obj, hotConfigMediaCodecFunc, videoWidth, videoHeight, videoBitRate,
                        (int) frameRate);

    if (EGL_NO_SURFACE != encoderSurface) {
        eglCore->releaseSurface(encoderSurface);
        encoderSurface = EGL_NO_SURFACE;
    }

    if (NULL != encoderWindow) {
		ANativeWindow_release(encoderWindow);
		encoderWindow = NULL;
	}
    jmethodID getEncodeSurfaceFromNativeFunc = env->GetMethodID(jcls, "getEncodeSurfaceFromNative",
                                                                "()Landroid/view/Surface;");
    jobject surface = env->CallObjectMethod(obj, getEncodeSurfaceFromNativeFunc);
    // 2 create window surface
    encoderWindow = ANativeWindow_fromSurface(env, surface);
    encoderSurface = eglCore->createWindowSurface(encoderWindow);

    if (needAttach) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }

    LOGI("leave hotConfigHWEncoder");
}


void HWEncoderAdapter::hotConfig(int maxBitrate, int avgBitrate, int fps) {
    this->videoBitRate = maxBitrate;
    isNeedHotConfigEncoder = true;
    resetFpsStartTimeIfNeed(fps);
}

void HWEncoderAdapter::destroyEncoder() {
    // 1 Stop Encoder Thread
    handler->postMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(mEncoderThreadId, 0);
    if (queue) {
        queue->abort();
        delete queue;
        queue = NULL;
    }
    delete handler;
    handler = NULL;
    LOGI("before destroyEncoder");
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = g_jvm->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }
    // 2 release encoder_
    this->destroyHWEncoder(env);
    // 3 release output memory
    if (outputBuffer)
        env->DeleteGlobalRef(outputBuffer);
    if (needAttach) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }
    LOGI("after destroyEncoder");
}

void HWEncoderAdapter::encode() {
    if (startTime == -1)
        startTime = getCurrentTime();

    if (fpsChangeTime == -1){
        fpsChangeTime = getCurrentTime();
    }

    if (handler->getQueueSize() > MAX_ENCODER_Q_SIZE) {
        LOGE("HWEncoderAdapter:dropped frame, encoder_ queue_ full");// See webrtc bug 2887.
        return;
    }
    int64_t curTime = getCurrentTime() - startTime;
    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fpsChangeTime) / 1000.0f * frameRate + 0.5f);
    if (expectedFrameCount < encodedFrameCount) {
//		LOGI("expectedFrameCount is %d while encodedFrameCount is %d", expectedFrameCount, encodedFrameCount);
        return;
    }
    encodedFrameCount++;
    if (EGL_NO_SURFACE != encoderSurface) {
        if (isNeedReconfigEncoder) {
            reConfigureHWEncoder();
            isNeedReconfigEncoder = false;
        }
        if (isNeedHotConfigEncoder) {
            hotConfigHWEncoder();
            isNeedHotConfigEncoder = false;
        }
        eglCore->makeCurrent(encoderSurface);
        renderer->renderToView(texId, videoWidth, videoHeight);
        eglCore->setPresentationTime(encoderSurface,
                                     ((khronos_stime_nanoseconds_t) curTime) * 1000000);
        handler->postMessage(new Message(FRAME_AVAILIBLE));
        if (!eglCore->swapBuffers(encoderSurface)) {
            LOGE("eglSwapBuffers(encoderSurface) returned error %d", eglGetError());
        }
    }
}

void HWEncoderAdapter::encodeLoop() {
    while (isEncoding) {
        Message *msg = NULL;
        if (queue->dequeueMessage(&msg, true) > 0) {
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->execute()) {
                isEncoding = false;
            }
            delete msg;
        }
    }
    LOGI("HWEncoderAdapter encode Thread ending...");
}

void HWEncoderAdapter::drainEncodedData() {
    JNIEnv *env;
    int status = 0;
    bool needAttach = false;
    status = g_jvm->GetEnv((void **) (&env), JNI_VERSION_1_4);
    if (status < 0) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
            return;
        }
        needAttach = true;
    }
    jclass jcls = env->GetObjectClass(obj);
    jmethodID drainEncoderFunc = env->GetMethodID(jcls, "pullH264StreamFromDrainEncoderFromNative",
                                                  "([B)J");
    long bufferSize = (long) env->CallLongMethod(obj, drainEncoderFunc, outputBuffer);
    byte *outputData = (uint8_t *) env->GetByteArrayElements(outputBuffer, 0);    // get data
    int size = (int) bufferSize;
//	LOGI("size is %d", size);

    jmethodID getLastPresentationTimeUsFunc = env->GetMethodID(jcls,
                                                               "getLastPresentationTimeUsFromNative",
                                                               "()J");
    long long lastPresentationTimeUs = (long long) env->CallLongMethod(obj,
                                                                       getLastPresentationTimeUsFunc);

    int timeMills = (int) (lastPresentationTimeUs / 1000.0f);

    // push to queue_
    int nalu_type = (outputData[4] & 0x1F);
    LOGI("nalu_type is %d... size is %d", nalu_type, size);
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
                packetPool->pushRecordingVideoPacketToQueue(videoPacket);
        }
        if (isSPSUnWriteFlag) {
            LiveVideoPacket *videoPacket = new LiveVideoPacket();
            videoPacket->buffer = new byte[size];
            memcpy(videoPacket->buffer, outputData, size);
            videoPacket->size = size;
            videoPacket->timeMills = timeMills;
            if (videoPacket->size > 0)
                packetPool->pushRecordingVideoPacketToQueue(videoPacket);
            isSPSUnWriteFlag = false;
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
            packetPool->pushRecordingVideoPacketToQueue(videoPacket);
    }
    env->ReleaseByteArrayElements(outputBuffer, (jbyte *) outputData, 0);
    if (needAttach) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
        }
    }

    if (startEncodeTime == 0){
        startEncodeTime = platform_4_live::getCurrentTimeSeconds();
    }
}
