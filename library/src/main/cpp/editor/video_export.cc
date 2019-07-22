//
// Created by wlanjie on 2019-06-15.
//

#include "error_code.h"
#include "video_export.h"
#include "soft_encoder_adapter.h"
#include "android_xlog.h"
#include "tools.h"
#include "error_code.h"
#include <math.h>

namespace trinity {

VideoExport::VideoExport() {
    export_ing = false;
    egl_core_ = nullptr;
    egl_surface_ = EGL_NO_SURFACE;
    media_decode_ = nullptr;
    state_event_ = nullptr;
    yuv_render_ = nullptr;
    export_index_ = 0;
    video_width_ = 0;
    video_height_ = 0;
    frame_width_ = 0;
    frame_height_ = 0;
    encoder_ = nullptr;
    audio_encoder_adapter_ = nullptr;
    packet_thread_ = nullptr;
    image_process_ = nullptr;
    current_time_ = 0;
    previous_time_ = 0;
    swr_context_ = nullptr;
    audio_buf = nullptr;
    audio_buf1 = nullptr;
    audio_samples_ = new short[8192];
    packet_pool_ = PacketPool::GetInstance();
    video_export_message_queue_ = new MessageQueue("Video Export Message Queue");
    video_export_handler_ = new VideoExportHandler(this, video_export_message_queue_);
    pthread_create(&export_message_thread_, nullptr, ExportMessageThread, this);

    output_stream_.open("/sdcard/export.pcm", std::ios_base::binary | std::ios_base::out);
}

VideoExport::~VideoExport() {
    video_export_handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(export_message_thread_, nullptr);
    output_stream_.close();
}

void* VideoExport::ExportMessageThread(void *context) {
    VideoExport* video_export = (VideoExport*) context;
    video_export->ProcessMessage();
    pthread_exit(0);
}

void VideoExport::ProcessMessage() {
    bool rendering = true;
    while (rendering) {
        Message *msg = NULL;
        if (video_export_message_queue_->DequeueMessage(&msg, true) > 0) {
            if (msg == NULL) {
                return;
            }
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                rendering = false;
            }
            delete msg;
        }
    }
}

int VideoExport::OnCompleteState(StateEvent *event) {
    VideoExport* video_export = (VideoExport*) event->context;
//    return video_export->OnComplete();
    video_export->video_export_handler_->PostMessage(new Message(kStartNextExport));
    return 0;
}

int VideoExport::OnComplete() {
    pthread_mutex_lock(&media_mutex_);
    FreeResource();
    if (clip_deque_.size() == 1) {
        export_ing = false;
    } else {
        export_index_++;
        if (export_index_ >= clip_deque_.size()) {
            export_ing = false;
        } else {
            previous_time_ = current_time_;
            MediaClip *clip = clip_deque_.at(export_index_);
            StartDecode(clip);
        }
    }
    pthread_cond_signal(&media_cond_);
    pthread_mutex_unlock(&media_mutex_);
    return 0;
}

void VideoExport::FreeResource() {
    av_decode_destroy(media_decode_);
    if (nullptr != media_decode_) {
        av_free(media_decode_);
        media_decode_ = nullptr;
    }
    if (nullptr != state_event_) {
        av_free(state_event_);
        state_event_ = nullptr;
    }
}

void VideoExport::StartDecode(trinity::MediaClip *clip) {
    media_decode_ = (MediaDecode*) av_malloc(sizeof(MediaDecode));
    memset(media_decode_, 0, sizeof(MediaDecode));
    media_decode_->start_time = clip->start_time;
    media_decode_->end_time = clip->end_time == 0 ? INT64_MAX : clip->end_time;

    state_event_ = (StateEvent*) av_malloc(sizeof(StateEvent));
    memset(state_event_, 0, sizeof(StateEvent));
    state_event_->context = this;
    state_event_->on_complete_event = OnCompleteState;
    media_decode_->state_event = state_event_;

    av_decode_start(media_decode_, clip->file_name);
}

int VideoExport::Export(const char *export_config, const char *path, int width, int height, int frame_rate,
                        int video_bit_rate, int sample_rate, int channel_count, int audio_bit_rate) {
    cJSON* json = cJSON_Parse(export_config);
    if (nullptr == json) {
        return EXPORT_CONFIG;
    }
    cJSON_Print(json);
    cJSON* clips = cJSON_GetObjectItem(json, "clips");
    if (nullptr == clips) {
        return CLIP_EMPTY;
    }
    int clip_size = cJSON_GetArraySize(clips);
    if (clip_size == 0) {
        return CLIP_EMPTY;
    }
    cJSON* item = clips->child;

    export_ing = true;
    packet_thread_ = new VideoConsumerThread();
    int ret = packet_thread_->Init(path, width, height, frame_rate, video_bit_rate, sample_rate, channel_count, audio_bit_rate, "libfdk_aac");
    if (ret < 0) {
        return ret;
    }
    PacketPool::GetInstance()->InitRecordingVideoPacketQueue();
    PacketPool::GetInstance()->InitAudioPacketQueue(44100);
    AudioPacketPool::GetInstance()->InitAudioPacketQueue();
    packet_thread_->StartAsync();

    video_width_ = width;
    video_height_ = height;

    for (int i = 0; i < clip_size; i++) {
        cJSON* path_item = cJSON_GetObjectItem(item, "path");
        cJSON* start_time_item = cJSON_GetObjectItem(item, "startTime");
        cJSON* end_time_item = cJSON_GetObjectItem(item, "endTime");
        item = item->next;

        MediaClip* export_clip = new MediaClip();
        export_clip->start_time = start_time_item->valueint;
        export_clip->end_time  = end_time_item->valueint;
        export_clip->file_name = path_item->valuestring;
        clip_deque_.push_back(export_clip);
    }

    cJSON* effects = cJSON_GetObjectItem(json, "effects");
    if (nullptr != effects) {
        int effect_size = cJSON_GetArraySize(effects);
        if (clip_size > 0) {
            cJSON* effects_child = effects->child;
            image_process_ = new ImageProcess();
            for (int i = 0; i < effect_size; ++i) {
                cJSON* type_item = cJSON_GetObjectItem(effects_child, "type");
                cJSON* start_time_item = cJSON_GetObjectItem(effects_child, "start_time");
                cJSON* end_time_item = cJSON_GetObjectItem(effects_child, "end_time");
                cJSON* split_count_item = cJSON_GetObjectItem(effects_child, "split_screen_count");
                if (type_item->valueint == SPLIT_SCREEN) {
                    image_process_->AddSplitScreenAction(split_count_item->valueint, start_time_item->valueint, end_time_item->valueint);
                }
                effects_child = effects_child->next;
            }
        }
    }

    encoder_ = new SoftEncoderAdapter(0);
    encoder_->Init(width, height, video_bit_rate, frame_rate);
    audio_encoder_adapter_ = new AudioEncoderAdapter();
    audio_encoder_adapter_->Init(packet_pool_, 44100, 1, 128 * 1024, "libfdk_aac");
    MediaClip* clip = clip_deque_.at(0);
    StartDecode(clip);
    pthread_mutex_init(&media_mutex_, nullptr);
    pthread_cond_init(&media_cond_, nullptr);
    pthread_create(&export_video_thread_, nullptr, ExportVideoThread, this);
    pthread_create(&export_audio_thread_, nullptr, ExportAudioThread, this);
    return 0;
}

void* VideoExport::ExportVideoThread(void* context) {
    VideoExport* video_export = (VideoExport*) (context);
    video_export->ProcessVideoExport();
    pthread_exit(0);
}

void VideoExport::ProcessVideoExport() {
    egl_core_ = new EGLCore();
    egl_core_->InitWithSharedContext();
    egl_surface_ = egl_core_->CreateOffscreenSurface(64, 64);
    if (nullptr == egl_surface_ || EGL_NO_SURFACE == egl_surface_) {
        return;
    }
    egl_core_->MakeCurrent(egl_surface_);
    FrameBuffer* frame_buffer = new FrameBuffer(video_width_, video_height_, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    encoder_->CreateEncoder(egl_core_, frame_buffer->GetTextureId());
    while (true) {
        pthread_mutex_lock(&media_mutex_);
        if (nullptr == media_decode_) {
            if (!export_ing) {
                pthread_mutex_unlock(&media_mutex_);
                break;
            }
            pthread_cond_wait(&media_cond_, &media_mutex_);
        }
        pthread_mutex_unlock(&media_mutex_);
        if (!export_ing) {
            break;
        }
        if (media_decode_->video_frame_queue.size == 0) {
            av_usleep(10000);
            continue;
        }
        if (frame_queue_nb_remaining(&media_decode_->video_frame_queue) == 0) {
            continue;
        }
        Frame* vp = frame_queue_peek(&media_decode_->video_frame_queue);
        if (vp->serial != media_decode_->video_packet_queue.serial) {
            frame_queue_next(&media_decode_->video_frame_queue);
            continue;
        }
        frame_queue_next(&media_decode_->video_frame_queue);
        Frame* frame = frame_queue_peek_last(&media_decode_->video_frame_queue);
        if (frame->frame != nullptr) {
            if (isnan(frame->pts)) {
                continue;
            }
            if (!frame->uploaded && frame->frame != nullptr) {
                int width = MIN(frame->frame->linesize[0], frame->frame->width);
                int height = frame->frame->height;
                if (frame_width_ != width || frame_height_ != height) {
                    frame_width_ = width;
                    frame_height_ = height;
                    if (nullptr != yuv_render_) {
                        delete yuv_render_;
                    }
                    yuv_render_ = new YuvRender(frame->width, frame->height, 0, 0, 0);
                }
                int texture_id = yuv_render_->DrawFrame(frame->frame);
                current_time_ = (uint64_t) (frame->frame->pts * av_q2d(media_decode_->video_stream->time_base) * 1000);
                if (image_process_ != nullptr) {
                    texture_id = image_process_->Process(texture_id, current_time_, frame->width, frame->height, 0, 0);
                }
                frame_buffer->OnDrawFrame(texture_id);
                if (!egl_core_->SwapBuffers(egl_surface_)) {
                    LOGE("eglSwapBuffers error: %d", eglGetError());
                }
                if (current_time_ == 0)  {
                    continue;
                }
                if (previous_time_ != 0) {
                    current_time_ = current_time_ + previous_time_;
                }
                encoder_->Encode(current_time_);
                frame->uploaded = 1;
            }
        }
    }
    encoder_->DestroyEncoder();
    delete encoder_;
    packet_thread_->Stop();
    PacketPool::GetInstance()->DestroyRecordingVideoPacketQueue();
    PacketPool::GetInstance()->DestroyAudioPacketQueue();
    AudioPacketPool::GetInstance()->DestroyAudioPacketQueue();
    delete packet_thread_;

    egl_core_->ReleaseSurface(egl_surface_);
    egl_core_->Release();
    egl_surface_ = EGL_NO_SURFACE;
    delete egl_core_;
    FreeResource();
}

void* VideoExport::ExportAudioThread(void *context) {
    VideoExport* video_export = (VideoExport*) context;
    video_export->ProcessAudioExport();
    pthread_exit(0);
}

void VideoExport::ProcessAudioExport() {
    while (true) {
        pthread_mutex_lock(&media_mutex_);
        if (nullptr == media_decode_) {
            LOGE("Audio wait");
            pthread_cond_wait(&media_cond_, &media_mutex_);
        }
        pthread_mutex_unlock(&media_mutex_);
        if (!export_ing) {
            break;
        }
        int audio_size = Resample();
        if (audio_size > 0) {
            AudioPacket *packet = new AudioPacket();
            output_stream_.write(reinterpret_cast<const char *>(audio_buf), audio_size);

            short *samples = new short[audio_size / sizeof(short)];
            memcpy(samples, audio_buf, audio_size);
            packet->buffer = samples;
            packet->size = audio_size / sizeof(short);
            packet_pool_->PushAudioPacketToQueue(packet);
        }
    }
    if (nullptr != swr_context_) {
        swr_free(&swr_context_);
    }
}

int VideoExport::Resample() {
    Frame* frame;
    do {
        frame = frame_queue_peek_readable(&media_decode_->sample_frame_queue);
        if (!frame) {
            LOGE("frame_queue_peek_readable error");
            return 0;
        }
        frame_queue_next(&media_decode_->sample_frame_queue);
    } while (frame->serial != media_decode_->audio_packet_queue.serial);

    int data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(frame->frame),
                                               frame->frame->nb_samples, (AVSampleFormat) frame->frame->format, 1);

    if (nullptr == swr_context_) {
        uint64_t dec_channel_layout = (frame->frame->channel_layout && av_frame_get_channels(frame->frame) == av_get_channel_layout_nb_channels(frame->frame->channel_layout)) ?
                                      frame->frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(frame->frame));
        swr_context_ = swr_alloc_set_opts(NULL, media_decode_->audio_tgt.channel_layout, media_decode_->audio_tgt.fmt, media_decode_->audio_tgt.freq,
                                          dec_channel_layout, (AVSampleFormat) frame->frame->format, frame->frame->sample_rate, 0, NULL);
        if (!swr_context_ || swr_init(swr_context_) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                   frame->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat) frame->frame->format), av_frame_get_channels(frame->frame),
                   media_decode_->audio_tgt.freq, av_get_sample_fmt_name(media_decode_->audio_tgt.fmt), media_decode_->audio_tgt.channels);
            swr_free(&swr_context_);
            return -1;
        }
    }
    unsigned int audio_buf1_size = 0;
    int wanted_nb_sample = frame->frame->nb_samples;
    int resample_data_size;
    if (swr_context_) {
        const uint8_t **in = (const uint8_t **) frame->frame->extended_data;
        uint8_t **out = &audio_buf1;
        int out_count = wanted_nb_sample * media_decode_->audio_tgt.freq / frame->frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, media_decode_->audio_tgt.channels, out_count, media_decode_->audio_tgt.fmt, 0);
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_sample != frame->frame->nb_samples) {
            if (swr_set_compensation(swr_context_, (wanted_nb_sample - frame->frame->nb_samples) * media_decode_->audio_tgt.freq / frame->frame->sample_rate,
                                     wanted_nb_sample * media_decode_->audio_tgt.freq / frame->frame->sample_rate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        av_fast_malloc(&audio_buf1, &audio_buf1_size, out_size);
        if (!audio_buf1) {
            return AVERROR(ENOMEM);
        }
        int len2 = swr_convert(swr_context_, out, out_count, in, frame->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too samll.\n");
            if (swr_init(swr_context_) < 0) {
                swr_free(&swr_context_);
            }
        }
        audio_buf = audio_buf1;
        resample_data_size = len2 * media_decode_->audio_tgt.channels * av_get_bytes_per_sample(media_decode_->audio_tgt.fmt);
    } else {
        audio_buf = frame->frame->data[0];
        resample_data_size = data_size;
    }
    return resample_data_size;
}

}