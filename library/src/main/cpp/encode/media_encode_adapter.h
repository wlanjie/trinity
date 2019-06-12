//
// Created by wlanjie on 2019/4/16.
//

#ifndef TRINITY_MEDIA_ENCODE_ADAPTER_H
#define TRINITY_MEDIA_ENCODE_ADAPTER_H

#include <jni.h>
#include "video_encoder_adapter.h"
#include "handler.h"
#include "video_packet_queue.h"
#include "opengl.h"

namespace trinity {

typedef enum {
    FRAME_AVAILABLE
} MediaEncoderType;

class MediaEncodeHandler;

class MediaEncodeAdapter : public VideoEncoderAdapter {
public:
    MediaEncodeAdapter(JavaVM* vm, jobject object);
    virtual ~MediaEncodeAdapter();

    virtual void CreateEncoder(EGLCore* core, int texture_id);

    virtual void DestroyEncoder();

    virtual void Encode();

    void DrainEncodeData();

private:
    bool encoding_;
    bool sps_write_flag_;
    JavaVM* vm_;
    jobject object_;
    EGLCore* core_;
    MediaEncodeHandler* handler_;
    MessageQueue* queue_;
    pthread_t encoder_thread_;
    EGLSurface encoder_surface_;
    ANativeWindow* encoder_window_;
    jbyteArray output_buffer_;
    int start_encode_time_;
    OpenGL* render_;
private:
    static void* EncoderThreadCallback(void* context);
    void EncodeLoop();
    void CreateMediaEncoder(JNIEnv* env);
    void DestroyMediaEncoder(JNIEnv* env);
};

class MediaEncodeHandler : public Handler {

public:
    MediaEncodeHandler(MediaEncodeAdapter* adapter, MessageQueue* queue) : Handler(queue) {
        adapter_ = adapter;
    }

    void HandleMessage(Message* msg) {
        int what = msg->GetWhat();
        switch (what) {
            case FRAME_AVAILABLE:
                adapter_->DrainEncodeData();
                break;
            default:
                break;
        }
    }

private:
    MediaEncodeAdapter* adapter_;
};

}

#endif //TRINITY_MEDIA_ENCODE_ADAPTER_H
