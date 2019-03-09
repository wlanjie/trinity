#ifndef RECORDING_HW_ENCODER_H
#define RECORDING_HW_ENCODER_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "../video_encoder_adapter.h"
#include "message_queue/handler.h"
#include "message_queue/message_queue.h"

typedef enum {
    FRAME_AVAILIBLE
} HWEncoderMSGType;

class HWEncoderHandler;

class HWEncoderAdapter : public VideoEncoderAdapter {
public:
    HWEncoderAdapter(JavaVM *g_jvm, jobject obj);

    virtual ~HWEncoderAdapter();

    virtual void CreateEncoder(EGLCore *eglCore, int inputTexId);

    virtual void DestroyEncoder();

    virtual void Encode();

    void ReConfigure(int maxBitRate, int avgBitRate, int fps);

    void ReConfigureHWEncoder();

    void HotConfig(int maxBitrate, int avgBitrate, int fps);

    void HotConfigHWEncoder();

    void DrainEncodedData();

private:
    bool encoding_;
    bool sps_unwrite_flag_;
    JavaVM *vm_;
    jobject obj_;
    EGLCore *egl_core_;
    HWEncoderHandler *handler_;
    MessageQueue *queue_;
    pthread_t encoder_thread_;
    static void *EncoderThreadCallback(void *myself);
    void EncodeLoop();
    EGLSurface encoder_surface_;
    ANativeWindow *encoder_window_;
    bool need_reconfig_encoder_;
    bool need_hot_config_encoder_;
    jbyteArray output_buffer_;
    int start_encode_time_ = 0;
    void DestroyHWEncoder(JNIEnv *env);
    void CreateHWEncoder(JNIEnv *env);
};

class HWEncoderHandler : public Handler {
private:
    HWEncoderAdapter *encoder_adapter_;
public:
    HWEncoderHandler(HWEncoderAdapter *encoderAdapter, MessageQueue *queue) :
            Handler(queue) {
        this->encoder_adapter_ = encoderAdapter;
    }

    void HandleMessage(Message *msg) {
        int what = msg->GetWhat();
        switch (what) {
            case FRAME_AVAILIBLE:
                encoder_adapter_->DrainEncodedData();
                break;
            default:
                break;
        }
    }
};

#endif // RECORDING_HW_ENCODER_H
