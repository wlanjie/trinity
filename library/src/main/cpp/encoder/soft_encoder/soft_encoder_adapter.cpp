#include "./soft_encoder_adapter.h"

#define LOG_TAG "SoftEncoderAdapter"

SoftEncoderAdapter::SoftEncoderAdapter(int strategy) :
        _msg(MSG_NONE), copyTexSurface(0) {
    hostGPUCopier = NULL;
    eglCore = NULL;
    encoder = NULL;
    outputTexId = -1;
    this->strategy = strategy;

    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mCondition, NULL);
    pthread_mutex_init(&previewThreadLock, NULL);
    pthread_cond_init(&previewThreadCondition, NULL);
}

SoftEncoderAdapter::~SoftEncoderAdapter() {
    pthread_mutex_destroy(&mLock);
    pthread_cond_destroy(&mCondition);
    pthread_mutex_destroy(&previewThreadLock);
    pthread_cond_destroy(&previewThreadCondition);
}

void SoftEncoderAdapter::createEncoder(EGLCore *eglCore, int inputTexId) {
	LOGI("enter createEncoder");
    this->loadTextureContext = eglCore->getContext();
    this->texId = inputTexId;
    pixelSize = videoWidth * videoHeight * PIXEL_BYTE_SIZE;
    hostGPUCopier = new HostGPUCopier();

    startTime = -1;
    fpsChangeTime = -1;

    encoder = new VideoX264Encoder(strategy);
    encoder->init(videoWidth, videoHeight, videoBitRate, frameRate, packetPool);
    yuy2PacketPool = LiveYUY2PacketPool::GetInstance();
    yuy2PacketPool->initYUY2PacketQueue();
    pthread_create(&x264EncoderThread, NULL, startEncodeThread, this);
    _msg = MSG_WINDOW_SET;
    pthread_create(&imageDownloadThread, NULL, startDownloadThread, this);

    LOGI("leave createEncoder");
}

void SoftEncoderAdapter::reConfigure(int maxBitRate, int avgBitRate, int fps) {
    if (encoder) {
        encoder->reConfigure(avgBitRate);
    }
}

void SoftEncoderAdapter::hotConfig(int maxBitrate, int avgBitrate, int fps) {
    if (encoder) {
        encoder->reConfigure(maxBitrate);
    }

    resetFpsStartTimeIfNeed(fps);
}

void SoftEncoderAdapter::encode() {
    while (_msg == MSG_WINDOW_SET || NULL == eglCore) {
        usleep(100 * 1000);
    }
    if (startTime == -1)
        startTime = getCurrentTime();

    if (fpsChangeTime == -1){
        fpsChangeTime = getCurrentTime();
    }

    int64_t curTime = getCurrentTime() - startTime;
    // need drop frames
    int expectedFrameCount = (int) ((getCurrentTime() - fpsChangeTime) / 1000.0f * frameRate + 0.5f);
    if (expectedFrameCount < encodedFrameCount) {
        LOGI("expectedFrameCount is %d while encodedFrameCount is %d", expectedFrameCount,
             encodedFrameCount);
        return;
    }
    encodedFrameCount++;
    pthread_mutex_lock(&previewThreadLock);
    pthread_mutex_lock(&mLock);
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
    pthread_cond_wait(&previewThreadCondition, &previewThreadLock);
    pthread_mutex_unlock(&previewThreadLock);
}

void SoftEncoderAdapter::destroyEncoder() {
    yuy2PacketPool->abortYUY2PacketQueue();
    pthread_join(x264EncoderThread, 0);
    yuy2PacketPool->destoryYUY2PacketQueue();
    if (NULL != encoder) {
        encoder->destroy();
        delete encoder;
        encoder = NULL;
    }

    pthread_mutex_lock(&mLock);
    _msg = MSG_RENDER_LOOP_EXIT;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
    pthread_join(imageDownloadThread, 0);
    if (NULL != hostGPUCopier) {
        hostGPUCopier->destroy();
    }
}

void *SoftEncoderAdapter::startDownloadThread(void *ptr) {
    SoftEncoderAdapter *softEncoderAdapter = (SoftEncoderAdapter *) ptr;
    softEncoderAdapter->renderLoop();
    pthread_exit(0);
    return 0;
}

void SoftEncoderAdapter::renderLoop() {
    bool renderingEnabled = true;
    while (renderingEnabled) {
        pthread_mutex_lock(&mLock);
        switch (_msg) {
            case MSG_WINDOW_SET:
                LOGI("receive msg MSG_WINDOW_SET");
                initialize();
                break;
            case MSG_RENDER_LOOP_EXIT:
                LOGI("receive msg MSG_RENDER_LOOP_EXIT");
                renderingEnabled = false;
                destroy();
                break;
            default:
                break;
        }
        _msg = MSG_NONE;
        if (NULL != eglCore) {
            eglCore->makeCurrent(copyTexSurface);
            this->loadTexture();
            pthread_cond_wait(&mCondition, &mLock);
        }
        pthread_mutex_unlock(&mLock);
    }
    return;
}

void SoftEncoderAdapter::loadTexture() {
    if (-1 == startTime) {
        return;
    }
    //1:拷贝纹理到我们的临时纹理
    int recordingDuration = getCurrentTime() - startTime;
    glViewport(0, 0, videoWidth, videoHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    checkGlError("glBindFramebuffer fbo_");
    long startTimeMills = getCurrentTime();
    renderer->renderToTexture(texId, outputTexId);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	LOGI("copy TexId waste timeMils 【%d】", (int)(getCurrentTime() - startTimeMills));
    this->signalPreviewThread();
    //2:从显存Download到内存
    startTimeMills = getCurrentTime();
    byte *packetBuffer = new byte[pixelSize];
    hostGPUCopier->copyYUY2Image(outputTexId, packetBuffer, videoWidth, videoHeight);
//	LOGI("Download Texture waste timeMils 【%d】", (int)(getCurrentTime() - startTimeMills));
    //3:构造LiveVideoPacket放到YUY2PacketPool里面
    LiveVideoPacket *videoPacket = new LiveVideoPacket();
    videoPacket->buffer = packetBuffer;
    videoPacket->size = pixelSize;
    videoPacket->timeMills = recordingDuration;
//	LOGI("recordingDuration 【%d】", recordingDuration);
    yuy2PacketPool->pushYUY2PacketToQueue(videoPacket);
}

void SoftEncoderAdapter::signalPreviewThread() {
    pthread_mutex_lock(&previewThreadLock);
    pthread_cond_signal(&previewThreadCondition);
    pthread_mutex_unlock(&previewThreadLock);
}

bool SoftEncoderAdapter::initialize() {
    eglCore = new EGLCore();
    eglCore->init(loadTextureContext);
    copyTexSurface = eglCore->createOffscreenSurface(videoWidth, videoHeight);
    eglCore->makeCurrent(copyTexSurface);
    renderer = new VideoGLSurfaceRender();
    renderer->init(videoWidth, videoHeight);
    glGenFramebuffers(1, &mFBO);
    //初始化outputTexId
    glGenTextures(1, &outputTexId);
    glBindTexture(GL_TEXTURE_2D, outputTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, videoWidth, videoHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void SoftEncoderAdapter::destroy() {
    if (NULL != eglCore) {
        eglCore->makeCurrent(copyTexSurface);
        if (mFBO) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &mFBO);
        }
        eglCore->releaseSurface(copyTexSurface);
        if (renderer) {
            LOGI("delete renderer_..");
            renderer->dealloc();
            delete renderer;
            renderer = NULL;
        }
        eglCore->release();
        eglCore = NULL;
    }
}

void *SoftEncoderAdapter::startEncodeThread(void *ptr) {
    SoftEncoderAdapter *softEncoderAdapter = (SoftEncoderAdapter *) ptr;
    softEncoderAdapter->startEncode();
    pthread_exit(0);
    return 0;
}

void SoftEncoderAdapter::startEncode() {
    LiveVideoPacket *videoPacket = NULL;
    while (true) {
//		LOGI("getYUY2PacketQueueSize is %d", yuy2PacketPool->getYUY2PacketQueueSize());
        if (yuy2PacketPool->getYUY2Packet(&videoPacket, true) < 0) {
            LOGI("yuy2PacketPool->getRecordingVideoPacket return negetive value...");
            break;
        }
        if (videoPacket) {
            //调用编码器编码这一帧数据
            encoder->encode(videoPacket);
            delete videoPacket;
            videoPacket = NULL;
        }
    }
}
