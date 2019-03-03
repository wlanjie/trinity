#include "audio_encoder.h"

#define LOG_TAG "AudioEncoder"

AudioEncoder::AudioEncoder() {
	encode_frame = NULL;
	avCodecContext = NULL;
	audio_next_pts = 0;
}

AudioEncoder::~AudioEncoder() {
}

int AudioEncoder::init(int bitRate, int channels, int sampleRate, const char * codec_name,
		int (*fill_pcm_frame_callback)(int16_t *, int, int, double*, void *context), void* context) {
	this->publishBitRate = bitRate;
	this->audioChannels = channels;
	this->audioSampleRate = sampleRate;
	this->fillPCMFrameCallback = fill_pcm_frame_callback;
    this->fillPCMFrameContext = context;
	av_register_all();
	this->alloc_audio_stream(codec_name);
	this->alloc_avframe();
	return 1;
}

int AudioEncoder::encode(LiveAudioPacket **audioPacket) {
//	LOGI("begin encode packet..................");
	 /** 1、调用注册的回调方法来填充音频的PCM数据 **/
	double presentationTimeMills = -1;
	int actualFillSampleSize = fillPCMFrameCallback((int16_t *) audio_samples_data[0], audio_nb_samples, audioChannels, &presentationTimeMills, fillPCMFrameContext);
	if (actualFillSampleSize == -1) {
		LOGI("fillPCMFrameCallback failed return actualFillSampleSize is %d \n", actualFillSampleSize);
		return -1;
	}
	if (actualFillSampleSize == 0) {
		return -1;
	}
	int actualFillFrameNum = actualFillSampleSize / audioChannels;
	int audioSamplesSize = actualFillFrameNum * audioChannels * sizeof(short);
	/** 2、将PCM数据按照编码器的格式编码到一个AVPacket中 **/
	AVRational time_base = {1, audioSampleRate};
	int ret;
	AVPacket pkt = { 0 };
	int got_packet;
	av_init_packet(&pkt);
	pkt.duration = (int) AV_NOPTS_VALUE;
	pkt.pts = pkt.dts = 0;
	encode_frame->nb_samples = actualFillFrameNum;
	avcodec_fill_audio_frame(encode_frame, avCodecContext->channels, avCodecContext->sample_fmt, audio_samples_data[0], audioSamplesSize, 0);
	encode_frame->pts = audio_next_pts;
	audio_next_pts += encode_frame->nb_samples;
//	int64_t calcuPTS = presentationTimeMills / 1000 / av_q2d(time_base) / audioChannels;
//	LOGI("encode_frame pts is %llu, calcuPTS is %llu", encode_frame->pts, calcuPTS);
	ret = avcodec_encode_audio2(avCodecContext, &pkt, encode_frame, &got_packet);
	if (ret < 0 || !got_packet) {
		LOGI("Error encoding audio frame: %s\n", av_err2str(ret));
		av_free_packet(&pkt);
		return ret;
	}
	if (got_packet) {
		pkt.pts = av_rescale_q(encode_frame->pts, avCodecContext->time_base, time_base);
		//转换为我们的LiveAudioPacket
		(*audioPacket) = new LiveAudioPacket();
		(*audioPacket)->data = new byte[pkt.size];
		memcpy((*audioPacket)->data, pkt.data, pkt.size);
		(*audioPacket)->size = pkt.size;
		(*audioPacket)->position = (float)(pkt.pts * av_q2d(time_base) * 1000.0f);
//		LOGI("size and position is {%f, %d}", (*audioPacket)->position, (*audioPacket)->size);
	}
	av_free_packet(&pkt);
	return ret;
//	LOGI("leave encode packet...");
}

void AudioEncoder::destroy() {
	LOGI("start Destroy!!!");
    if (NULL != audio_samples_data[0]) {
        av_free(audio_samples_data[0]);
    }
	if (NULL != encode_frame) {
		av_free(encode_frame);
	}
	if (NULL != avCodecContext) {
		avcodec_close(avCodecContext);
		av_free(avCodecContext);
	}
	LOGI("end Destroy!!!");
}

int AudioEncoder::alloc_audio_stream(const char * codec_name) {
	AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		LOGI("Couldn't find a valid audio codec By Codec Name %s", codec_name);
		return -1;
	}
	avCodecContext = avcodec_alloc_context3(codec);
	avCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
	avCodecContext->sample_rate = audioSampleRate;
	if (publishBitRate > 0) {
		avCodecContext->bit_rate = publishBitRate;
	} else {
		avCodecContext->bit_rate = PUBLISH_BITE_RATE;
	}
	avCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
	LOGI("audioChannels is %d", audioChannels);
	LOGI("AV_SAMPLE_FMT_S16 is %d", AV_SAMPLE_FMT_S16);
	avCodecContext->channel_layout = audioChannels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	avCodecContext->channels = av_get_channel_layout_nb_channels(avCodecContext->channel_layout);
	avCodecContext->profile = FF_PROFILE_AAC_LOW;
	LOGI("avCodecContext->channels is %d", avCodecContext->channels);
	avCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
	avCodecContext->codec_id = codec->id;
	if (avcodec_open2(avCodecContext, codec, NULL) < 0) {
		LOGI("Couldn't open codec");
		return -2;
	}
	return 0;

}

int AudioEncoder::alloc_avframe() {
	int ret = 0;
	encode_frame = avcodec_alloc_frame();
	if (!encode_frame) {
		LOGI("Could not allocate audio frame\n");
		return -1;
	}
	encode_frame->nb_samples = avCodecContext->frame_size;
	encode_frame->format = avCodecContext->sample_fmt;
	encode_frame->channel_layout = avCodecContext->channel_layout;
	encode_frame->sample_rate = avCodecContext->sample_rate;

    /**
     * 这个地方原来是10000就导致了编码frame的时候有9个2048一个1568 其实编码解码是没有问题的
     * 但是在我们后续的sox处理的时候就会有问题，这里一定要注意注意 所以改成了10240
     */
    audio_nb_samples = avCodecContext->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ? 10240 : avCodecContext->frame_size;
    int src_samples_linesize;
    ret = av_samples_alloc_array_and_samples(&audio_samples_data, &src_samples_linesize, avCodecContext->channels, audio_nb_samples, avCodecContext->sample_fmt, 0);
    if (ret < 0) {
        LOGI("Could not allocate source samples\n");
        return -1;
    }
    audio_samples_size = av_samples_get_buffer_size(NULL, avCodecContext->channels, audio_nb_samples, avCodecContext->sample_fmt, 0);
	return ret;
}
