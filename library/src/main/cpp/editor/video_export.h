//
// Created by wlanjie on 2019-06-15.
//

#ifndef TRINITY_VIDEO_EXPORT_H
#define TRINITY_VIDEO_EXPORT_H

extern "C" {
#include "ffplay.h"
#include "cJSON.h"
};

#include <deque>

#include "video_encoder_adapter.h"
#include "video_consumer_thread.h"
#include "yuv_render.h"
#include "image_process.h"

namespace trinity {

using namespace std;

typedef struct {
    char* file_name;
    int64_t start_time;
    int64_t end_time;
} MediaClip;

class VideoExport {
public:
    VideoExport();
    ~VideoExport();

    int Export(const char* export_config, const char* path,
            int width, int height, int frame_rate, int video_bit_rate,
            int sample_rate, int channel_count, int audio_bit_rate);

    int Export(deque<MediaClip*> clips, const char* path, int width, int height, int frame_rate, int video_bit_rate,
               int sample_rate, int channel_count, int audio_bit_rate);

private:
    static void* ExportThread(void* context);
    void ProcessExport();

private:
    int FrameQueueInit(FrameQueue *f, PacketQueue *pktq, int maxSize, int keepLast);
    Frame* FrameQueuePeekWritable(FrameQueue* f);
    Frame* FrameQueuePeekReadable(FrameQueue* f);
    void FrameQueuePush(FrameQueue* f);
    Frame* FrameQueuePeek(FrameQueue* f);
    int FrameQueueNbRemaining(FrameQueue* f);
    Frame* FrameQueuePeekLast(FrameQueue* f);
    Frame* FrameQueuePeekNext(FrameQueue* f);
    void FrameQueueUnrefItem(Frame* vp);
    void FrameQueueNext(FrameQueue* f);
    void FrameQueueSignal(FrameQueue* f);
    void FrameQueueDestroy(FrameQueue* f);
    int PacketQueueInit(PacketQueue* q);
    int PacketQueueGet(PacketQueue* q, AVPacket* pkt, int block, int* serial);
    int PacketQueuePut(PacketQueue* q, AVPacket* pkt);
    int PacketQueuePutPrivate(PacketQueue* q, AVPacket* packet);
    int PacketQueuePutNullPacket(PacketQueue* q, int streamIndex);
    void PacketQueueStart(PacketQueue* q);
    void PacketQueueAbort(PacketQueue* q);
    void PacketQueueFlush(PacketQueue* q);
    void PacketQueueDestroy(PacketQueue* q);


    void DecoderInit(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, pthread_cond_t empty_queue_cond);
    int DecoderStart(Decoder *d, void* (*fn) (void*), void *arg);
    int StreamCommonOpen(int stream_index);
    int ReadFrame();
    int StreamOpen(VideoState* is, char* file_name);
    void StreamClose(VideoState* is);

    int GetVideoFrame(VideoState *is, AVFrame *frame);
    int QueuePicture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    int DecodeFrame(VideoState* is, Decoder *d, AVFrame *frame, AVSubtitle *sub);
    int DecoderAudio();
    int DecoderVideo();
    static void* ReadThread(void* arg);
    static void* AudioThread(void* arg);
    static void* VideoThread(void* arg);
private:
    AVPacket flush_packet_;

    pthread_t export_thread_;
    deque<MediaClip*> clip_deque_;
    bool export_ing;
    EGLCore* egl_core_;
    EGLSurface egl_surface_;
    VideoState* video_state_;
    int export_index_;
    int video_width_;
    int video_height_;
    int frame_width_;
    int frame_height_;
    YuvRender* yuv_render_;
    ImageProcess* image_process_;
    VideoEncoderAdapter* encoder_;
    VideoConsumerThread* packet_thread_;
};

}

#endif //TRINITY_VIDEO_EXPORT_H
