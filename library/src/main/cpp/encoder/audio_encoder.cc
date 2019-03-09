#include "audio_encoder.h"

#define LOG_TAG "AudioEncoder"

AudioEncoder::AudioEncoder() {
	encode_frame_ = NULL;
	codec_context_ = NULL;
	audio_next_pts_ = 0;
}

AudioEncoder::~AudioEncoder() {
}

int AudioEncoder::Init(int bitRate, int channels, int sampleRate, const char *codec_name,
					   int (*FillPCMFrameCallback)(int16_t *, int, int, double *, void *context), void *context) {
	this->bit_rate_ = bitRate;
	this->channels_ = channels;
	this->sample_rate_ = sampleRate;
	this->fill_pcm_frame_callback_ = FillPCMFrameCallback;
    this->fill_pcm_frame_context_ = context;
	av_register_all();
	this->AllocAudioStream(codec_name);
	this->AllocAVFrame();
	return 1;
}

int AudioEncoder::Encode(LiveAudioPacket **audioPacket) {
//	LOGI("begin Encode packet_..................");
	 /** 1、调用注册的回调方法来填充音频的PCM数据 **/
	double presentationTimeMills = -1;
	int actualFillSampleSize = fill_pcm_frame_callback_((int16_t *) audio_samples_data_[0], audio_nb_samples_, channels_, &presentationTimeMills, fill_pcm_frame_context_);
	if (actualFillSampleSize == -1) {
		LOGI("fill_pcm_frame_callback_ failed return actualFillSampleSize is %d \n", actualFillSampleSize);
		return -1;
	}
	if (actualFillSampleSize == 0) {
		return -1;
	}
	int actualFillFrameNum = actualFillSampleSize / channels_;
	int audioSamplesSize = actualFillFrameNum * channels_ * sizeof(short);
	/** 2、将PCM数据按照编码器的格式编码到一个AVPacket中 **/
	AVRational time_base = {1, sample_rate_};
	int ret;
	AVPacket pkt = { 0 };
	int got_packet;
	av_init_packet(&pkt);
	pkt.duration = (int) AV_NOPTS_VALUE;
	pkt.pts = pkt.dts = 0;
	encode_frame_->nb_samples = actualFillFrameNum;
	avcodec_fill_audio_frame(encode_frame_, codec_context_->channels, codec_context_->sample_fmt, audio_samples_data_[0], audioSamplesSize, 0);
	encode_frame_->pts = audio_next_pts_;
	audio_next_pts_ += encode_frame_->nb_samples;
//	int64_t calcuPTS = presentationTimeMills / 1000 / av_q2d(time_base) / channels_;
//	LOGI("encode_frame_ pts is %llu, calcuPTS is %llu", encode_frame_->pts, calcuPTS);
	ret = avcodec_encode_audio2(codec_context_, &pkt, encode_frame_, &got_packet);
	if (ret < 0 || !got_packet) {
		LOGI("Error encoding audio frame_: %s\n", av_err2str(ret));
		av_free_packet(&pkt);
		return ret;
	}
	if (got_packet) {
		pkt.pts = av_rescale_q(encode_frame_->pts, codec_context_->time_base, time_base);
		//转换为我们的LiveAudioPacket
		(*audioPacket) = new LiveAudioPacket();
		(*audioPacket)->data = new byte[pkt.size];
		memcpy((*audioPacket)->data, pkt.data, pkt.size);
		(*audioPacket)->size = pkt.size;
		(*audioPacket)->position = (float)(pkt.pts * av_q2d(time_base) * 1000.0f);
//		LOGI("Size and position_ is {%f, %d}", (*audioPacket)->position_, (*audioPacket)->Size);
	}
	av_free_packet(&pkt);
	return ret;
//	LOGI("leave Encode packet_...");
}

void AudioEncoder::Destroy() {
	LOGI("Start Destroy!!!");
    if (NULL != audio_samples_data_[0]) {
        av_free(audio_samples_data_[0]);
    }
	if (NULL != encode_frame_) {
		av_free(encode_frame_);
	}
	if (NULL != codec_context_) {
		avcodec_close(codec_context_);
		av_free(codec_context_);
	}
	LOGI("end Destroy!!!");
}

int AudioEncoder::AllocAudioStream(const char *codec_name) {
	AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		LOGI("Couldn't find a valid audio codec By Codec Name %s", codec_name);
		return -1;
	}
	codec_context_ = avcodec_alloc_context3(codec);
	codec_context_->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_context_->sample_rate = sample_rate_;
	if (bit_rate_ > 0) {
		codec_context_->bit_rate = bit_rate_;
	} else {
		codec_context_->bit_rate = PUBLISH_BITE_RATE;
	}
	codec_context_->sample_fmt = AV_SAMPLE_FMT_S16;
	LOGI("channels_ is %d", channels_);
	LOGI("AV_SAMPLE_FMT_S16 is %d", AV_SAMPLE_FMT_S16);
	codec_context_->channel_layout = channels_ == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	codec_context_->channels = av_get_channel_layout_nb_channels(codec_context_->channel_layout);
	codec_context_->profile = FF_PROFILE_AAC_LOW;
	LOGI("codec_context_->channels_ is %d", codec_context_->channels);
	codec_context_->flags |= CODEC_FLAG_GLOBAL_HEADER;
	codec_context_->codec_id = codec->id;
	if (avcodec_open2(codec_context_, codec, NULL) < 0) {
		LOGI("Couldn't open codec");
		return -2;
	}
	return 0;

}

int AudioEncoder::AllocAVFrame() {
	int ret = 0;
	encode_frame_ = avcodec_alloc_frame();
	if (!encode_frame_) {
		LOGI("Could not allocate audio frame_\n");
		return -1;
	}
	encode_frame_->nb_samples = codec_context_->frame_size;
	encode_frame_->format = codec_context_->sample_fmt;
	encode_frame_->channel_layout = codec_context_->channel_layout;
	encode_frame_->sample_rate = codec_context_->sample_rate;

    /**
     * 这个地方原来是10000就导致了编码frame的时候有9个2048一个1568 其实编码解码是没有问题的
     * 但是在我们后续的sox处理的时候就会有问题，这里一定要注意注意 所以改成了10240
     */
    audio_nb_samples_ = codec_context_->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ? 10240 : codec_context_->frame_size;
    int src_samples_linesize;
    ret = av_samples_alloc_array_and_samples(&audio_samples_data_, &src_samples_linesize, codec_context_->channels, audio_nb_samples_, codec_context_->sample_fmt, 0);
    if (ret < 0) {
        LOGI("Could not allocate source samples\n");
        return -1;
    }
    audio_samples_size_ = av_samples_get_buffer_size(NULL, codec_context_->channels, audio_nb_samples_, codec_context_->sample_fmt, 0);
	return ret;
}
