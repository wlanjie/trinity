//
// Created by wlanjie on 2019/4/20.
//

#include "music_decoder.h"
#include "android_xlog.h"
#include "tools.h"

#define OUT_PUT_CHANNELS 2

namespace trinity {

MusicDecoder::MusicDecoder() {
    path_ = nullptr;
    format_context_ = nullptr;
    codec_context_ = nullptr;
    audio_frame_ = nullptr;
    swr_context_ = nullptr;
    swr_buffer_ = nullptr;
    seek_req_ = false;
    seek_resp_ = false;
}

MusicDecoder::~MusicDecoder() {
    if (nullptr != path_) {
        delete[] path_;
        path_ = nullptr;
    }
}

int MusicDecoder::Init(const char *path, int packet_buffer_size) {
    int ret = Init(path);
    packet_buffer_size_ = packet_buffer_size;
    return ret;
}

int MusicDecoder::Init(const char *path) {
    audio_buffer_ = nullptr;
    position_ = -1.0f;
    audio_buffer_cursor_ = 0;
    audio_buffer_size_ = 0;
    swr_context_ = nullptr;
    swr_buffer_ = nullptr;
    swr_buffer_size_ = 0;
    seek_success_read_frame_success_ = true;
    need_first_frame_correct_flag_ = true;
    first_frame_correction_in_secs_ = 0.0f;
    format_context_ = avformat_alloc_context();
    if (nullptr == path_) {
        int length = strlen(path);
        path_ = new char[length + 1];
        memset(path_, 0, length + 1);
        memcpy(path_, path, length + 1);
    }
    int ret= avformat_open_input(&format_context_, path, nullptr, nullptr);
    if (ret != 0) {
        LOGE("open file error: %s", av_err2str(ret));
        Destroy();
        return -1;
    }
    ret = avformat_find_stream_info(format_context_, nullptr);
    if (ret != 0) {
        Destroy();
        return -1;
    }
    stream_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index_ == -1) {
        LOGE("find audio stream index error");
        Destroy();
        return -1;
    }

    AVStream* audio_stream = format_context_->streams[stream_index_];
    if (audio_stream->time_base.den && audio_stream->time_base.num) {
        time_base_ = av_q2d(audio_stream->time_base);
    } else if (audio_stream->codec->time_base.den && audio_stream->codec->time_base.num) {
        time_base_ = av_q2d(audio_stream->codec->time_base);
    }
    codec_context_ = audio_stream->codec;
    AVCodec* codec = avcodec_find_decoder(codec_context_->codec_id);
    if (codec == nullptr) {
        Destroy();
        return -1;
    }
    ret = avcodec_open2(codec_context_, codec, nullptr);
    if (ret < 0) {
        Destroy();
        return -1;
    }
    if (!AudioCodecIsSupported()) {
        swr_context_ = swr_alloc_set_opts(NULL, av_get_default_channel_layout(OUT_PUT_CHANNELS), AV_SAMPLE_FMT_S16, codec_context_->sample_rate,
                                          av_get_default_channel_layout(codec_context_->channels), codec_context_->sample_fmt, codec_context_->sample_rate, 0, NULL);
        if (!swr_context_ || swr_init(swr_context_)) {
            if (swr_context_)
                swr_free(&swr_context_);
            avcodec_close(codec_context_);
            LOGE("Init resampler_ failed...");
            return -1;
        }
    }
    audio_frame_ = av_frame_alloc();

    output_stream_.open("/sdcard/music.pcm", std::ios_base::binary | std::ios_base::out);
    return 0;
}

void MusicDecoder::SetPacketBufferSize(int packet_buffer_size) {
    packet_buffer_size_ = packet_buffer_size;
}

AudioPacket *MusicDecoder::DecodePacket() {
    short* samples = new short[packet_buffer_size_];
    int stereoSampleSize = ReadSamples(samples, packet_buffer_size_);
    AudioPacket* samplePacket = new AudioPacket();
    if (stereoSampleSize > 0) {
        samplePacket->buffer = samples;
        samplePacket->size = stereoSampleSize;
        samplePacket->position = position_;
    } else {
        samplePacket->size = -1;
    }
    return samplePacket;
}

void MusicDecoder::SeekFrame() {
    float targetPosition = seek_seconds_;
    float currentPosition = position_;
    float frameDuration = duration_;
    if(targetPosition < currentPosition){
        Destroy();
        Init(path_);
        //TODO:这里GT的测试样本会差距25ms 不会累加
        currentPosition = 0.0;
    }
    int readFrameCode = -1;
    while (true) {
        av_init_packet(&packet_);
        readFrameCode = av_read_frame(format_context_, &packet_);
        if (readFrameCode >= 0) {
            currentPosition += frameDuration;
            if (currentPosition >= targetPosition) {
                break;
            }
        }
        av_free_packet(&packet_);
    }
    seek_resp_ = true;
    seek_req_ = false;
    seek_success_read_frame_success_ = false;
}

void MusicDecoder::Destroy() {
    if (nullptr != swr_buffer_) {
        free(swr_buffer_);
        swr_buffer_ = NULL;
        swr_buffer_size_ = 0;
    }
    if (nullptr != swr_context_) {
        swr_free(&swr_context_);
        swr_context_ = NULL;
    }
    if (nullptr != audio_frame_) {
        av_free (audio_frame_);
        audio_frame_ = NULL;
    }
    if (nullptr != codec_context_) {
        avcodec_close(codec_context_);
        codec_context_ = NULL;
    }
    if (nullptr != format_context_) {
        LOGI("leave LiveReceiver::Destroy");
        avformat_close_input(&format_context_);
        format_context_ = NULL;
    }
    output_stream_.close();
}

int MusicDecoder::GetSampleRate() {
    int sampleRate = -1;
    if(nullptr != codec_context_){
        sampleRate = codec_context_->sample_rate;
    }
    return sampleRate;
}

void MusicDecoder::SetSeekReq(bool seek_req) {
    seek_req_ = seek_req;
    if (seek_req) {
        seek_resp_ = false;
    }
}

bool MusicDecoder::HasSeekReq() {
    return seek_req_;
}

bool MusicDecoder::HasSeekResp() {
    return seek_resp_;
}

void MusicDecoder::SetPosition(float seconds) {
    actual_seek_position_ = -1;
    seek_seconds_ = seconds;
    seek_req_ = true;
    seek_resp_ = false;
}

float MusicDecoder::GetActualSeekPosition() {
    float ret = actual_seek_position_;
    if (ret != -1) {
        actual_seek_position_ = -1;
    }
    return ret;
}

int MusicDecoder::ReadSamples(short *samples, int size) {
    if(seek_req_){
        audio_buffer_cursor_ = audio_buffer_size_;
    }
    int sampleSize = size;
    while(size > 0){
        if(audio_buffer_cursor_ < audio_buffer_size_){
            int audioBufferDataSize = audio_buffer_size_ - audio_buffer_cursor_;
            int copySize = MIN(size, audioBufferDataSize);
            memcpy(samples + (sampleSize - size), audio_buffer_ + audio_buffer_cursor_, copySize * 2);
            size -= copySize;
            audio_buffer_cursor_ += copySize;
        } else{
            if(ReadFrame() < 0){
                break;
            }
        }
    }
    int fillSize = sampleSize - size;
    if(fillSize == 0){
        return -1;
    }
    return fillSize;
}

int MusicDecoder::ReadFrame() {
    if(seek_req_){
        this->SeekFrame();
    }
    int ret = 1;
    av_init_packet(&packet_);
    int gotframe = 0;
    int readFrameCode = -1;
    while (true) {
        readFrameCode = av_read_frame(format_context_, &packet_);
        if (readFrameCode >= 0) {
            if (packet_.stream_index == stream_index_) {
                int len = avcodec_decode_audio4(codec_context_, audio_frame_,
                                                &gotframe, &packet_);
                if (len < 0) {
                    LOGI("Decode audio error, skip packet_");
                }
                if (gotframe) {
                    int numChannels = OUT_PUT_CHANNELS;
                    int numFrames = 0;
                    void * audioData;
                    if (swr_context_) {
                        const int ratio = 2;
                        const int bufSize = av_samples_get_buffer_size(NULL,
                                                                       numChannels, audio_frame_->nb_samples * ratio,
                                                                       AV_SAMPLE_FMT_S16, 1);
                        if (!swr_buffer_ || swr_buffer_size_ < bufSize) {
                            swr_buffer_size_ = bufSize;
                            swr_buffer_ = realloc(swr_buffer_, swr_buffer_size_);
                        }
                        uint8_t *outbuf[2] = { (uint8_t*) swr_buffer_, NULL };
                        numFrames = swr_convert(swr_context_, outbuf,
                                                audio_frame_->nb_samples * ratio,
                                                (const uint8_t **) audio_frame_->data,
                                                audio_frame_->nb_samples);
                        if (numFrames < 0) {
                            LOGE("fail resample audio");
                            ret = -1;
                            break;
                        }
                        audioData = swr_buffer_;
                    } else {
                        if (codec_context_->sample_fmt != AV_SAMPLE_FMT_S16) {
                            LOGE("bucheck, audio format is invalid");
                            ret = -1;
                            break;
                        }
                        audioData = audio_frame_->data[0];
                        numFrames = audio_frame_->nb_samples;
                    }
                    if(need_first_frame_correct_flag_ && position_ >= 0){
                        float expectedPosition = position_ + duration_;
                        float actualPosition = av_frame_get_best_effort_timestamp(audio_frame_) * time_base_;
                        first_frame_correction_in_secs_ = actualPosition - expectedPosition;
                        need_first_frame_correct_flag_ = false;
                    }
                    duration_ = av_frame_get_pkt_duration(audio_frame_) * time_base_;
                    position_ = av_frame_get_best_effort_timestamp(audio_frame_) * time_base_ - first_frame_correction_in_secs_;
                    if (!seek_success_read_frame_success_) {
                        LOGI("position_ is %.6f", position_);
                        actual_seek_position_ = position_;
                        seek_success_read_frame_success_ = true;
                    }
                    audio_buffer_size_ = numFrames * numChannels;
                    audio_buffer_ = (short*) audioData;
                    audio_buffer_cursor_ = 0;
                    output_stream_.write(static_cast<const char *>(audioData), audio_buffer_size_ * 2 );
                    break;
                }
            }
        } else {
            ret = -1;
            break;
        }
    }
    av_free_packet(&packet_);
    return 0;
}

bool MusicDecoder::AudioCodecIsSupported() {
    return codec_context_->sample_fmt == AV_SAMPLE_FMT_S16;
}

}
