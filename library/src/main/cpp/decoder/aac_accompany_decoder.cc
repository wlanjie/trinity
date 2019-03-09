#include "aac_accompany_decoder.h"

#define LOG_TAG "AACAccompanyDecoder"

AACAccompanyDecoder::AACAccompanyDecoder() {
	accompany_file_path_ = NULL;
	codec_context_ = NULL;
	format_context_ = NULL;
	swr_buffer_ = NULL;
	swr_context_ = NULL;
	audio_frame_ = NULL;
}

AACAccompanyDecoder::~AACAccompanyDecoder() {
	if(NULL != accompany_file_path_){
		delete[] accompany_file_path_;
		accompany_file_path_ = NULL;
	}
}

int AACAccompanyDecoder::GetMusicMeta(const char *fileString, int *metaData) {
	Init(fileString);
	int sampleRate = codec_context_->sample_rate;
	LOGI("sampleRate is %d", sampleRate);
	int bitRate = codec_context_->bit_rate;
	LOGI("bitRate is %d", bitRate);
	Destroy();
	metaData[0] = sampleRate;
	metaData[1] = bitRate;
	return 0;
}

void AACAccompanyDecoder::Init(const char *fileString, int packetBufferSizeParam){
	Init(fileString);
	packet_buffer_size_ = packetBufferSizeParam;
}

void AACAccompanyDecoder::SetPacketBufferSize(int packetBufferSizeParam){
	packet_buffer_size_ = packetBufferSizeParam;
}

int AACAccompanyDecoder::Init(const char *audioFile) {
	LOGI("enter AACAccompanyDecoder::Init");
	audio_buffer_ = NULL;
	position_ = -1.0f;
	audio_buffer_cursor_ = 0;
	audio_buffer_size_ = 0;
	swr_context_ = NULL;
	swr_buffer_ = NULL;
	swr_buffer_size_ = 0;
	seek_success_read_frame_success_ = true;
	need_first_frame_correct_flag_ = true;
	first_frame_correction_in_secs_ = 0.0f;
	format_context_ = avformat_alloc_context();
	LOGI("open accompany file %s....", audioFile);

	if(NULL == accompany_file_path_){
		int length = strlen(audioFile);
		accompany_file_path_ = new char[length + 1];
		//由于最后一个是'\0' 所以memset的长度要设置为length+1
		memset(accompany_file_path_, 0, length + 1);
		memcpy(accompany_file_path_, audioFile, length + 1);
	}

	int result = avformat_open_input(&format_context_, audioFile, NULL, NULL);
	if (result != 0) {
		LOGI("can't open file %s result is %d", audioFile, result);
		format_context_ = NULL;
		return -1;
	} else {
		LOGI("open file %s success and result is %d", audioFile, result);
	}
	format_context_->max_analyze_duration = 50000;
	//检查在文件中的流的信息
	result = avformat_find_stream_info(format_context_, NULL);
	if (result < 0) {
		LOGI("fail avformat_find_stream_info result is %d", result);
		return -1;
	} else {
		LOGI("sucess avformat_find_stream_info result is %d", result);
	}
	stream_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	LOGI("stream_index_ is %d", stream_index_);
	// 没有音频
	if (stream_index_ == -1) {
		LOGI("no audio stream");
		return -1;
	}
	//音频流
	AVStream* audioStream = format_context_->streams[stream_index_];
	if (audioStream->time_base.den && audioStream->time_base.num)
		time_base_ = av_q2d(audioStream->time_base);
	else if (audioStream->codec->time_base.den && audioStream->codec->time_base.num)
		time_base_ = av_q2d(audioStream->codec->time_base);
	//获得音频流的解码器上下文
	codec_context_ = audioStream->codec;
	// 根据解码器上下文找到解码器
	LOGI("codec_context_->codec_id is %d AV_CODEC_ID_AAC is %d", codec_context_->codec_id, AV_CODEC_ID_AAC);
	AVCodec * avCodec = avcodec_find_decoder(codec_context_->codec_id);
	if (avCodec == NULL) {
		LOGI("Unsupported codec ");
		return -1;
	}
	// 打开解码器
	result = avcodec_open2(codec_context_, avCodec, NULL);
	if (result < 0) {
		LOGI("fail avformat_find_stream_info result is %d", result);
		return -1;
	} else {
		LOGI("sucess avformat_find_stream_info result is %d", result);
	}
	//4、判断是否需要resampler
	if (!AudioCodecIsSupported()) {
		LOGI("because of audio Codec Is Not Supported so we will Init swresampler...");
		/**
		 * 初始化resampler
		 * @param s               Swr context, can be NULL
		 * @param out_ch_layout   output channel layout (AV_CH_LAYOUT_*)
		 * @param out_sample_fmt  output sample format (AV_SAMPLE_FMT_*).
		 * @param out_sample_rate output sample rate (frequency in Hz)
		 * @param in_ch_layout    input channel layout (AV_CH_LAYOUT_*)
		 * @param in_sample_fmt   input sample format (AV_SAMPLE_FMT_*).
		 * @param in_sample_rate  input sample rate (frequency in Hz)
		 * @param log_offset      logging level offset
		 * @param log_ctx         parent logging context, can be NULL
		 */
		swr_context_ = swr_alloc_set_opts(NULL, av_get_default_channel_layout(OUT_PUT_CHANNELS), AV_SAMPLE_FMT_S16, codec_context_->sample_rate,
				av_get_default_channel_layout(codec_context_->channels), codec_context_->sample_fmt, codec_context_->sample_rate, 0, NULL);
		if (!swr_context_ || swr_init(swr_context_)) {
			if (swr_context_)
				swr_free(&swr_context_);
			avcodec_close(codec_context_);
			LOGI("Init resampler_ failed...");
			return -1;
		}
	}
	LOGI("channels_ is %d sampleRate is %d", codec_context_->channels, codec_context_->sample_rate);
	audio_frame_ = avcodec_alloc_frame();
//	LOGI("leave AACAccompanyDecoder::Init");
	return 1;
}

bool AACAccompanyDecoder::AudioCodecIsSupported() {
	return codec_context_->sample_fmt == AV_SAMPLE_FMT_S16;
}

LiveAudioPacket* AACAccompanyDecoder::DecodePacket(){
//	LOGI("MadDecoder::DecodePacket packet_buffer_size_ is %d", packet_buffer_size_);
	short* samples = new short[packet_buffer_size_];
//	LOGI("accompanyPacket buffer_'s addr is %x", samples);
	int stereoSampleSize = ReadSamples(samples, packet_buffer_size_);
	LiveAudioPacket* samplePacket = new LiveAudioPacket();
	if (stereoSampleSize > 0) {
		//构造成一个packet
		samplePacket->buffer = samples;
		samplePacket->size = stereoSampleSize;
		/** 这里由于每一个packet的大小不一样有可能是200ms 但是这样子position就有可能不准确了 **/
		samplePacket->position = position_;
	} else {
		samplePacket->size = -1;
	}
	return samplePacket;
}

int AACAccompanyDecoder::ReadSamples(short *samples, int size) {
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
//		LOGI("Size is %d", Size);
	}
	int fillSize = sampleSize - size;
	if(fillSize == 0){
		return -1;
	}
	return fillSize;
}


void AACAccompanyDecoder::SeekFrame(){
	LOGI("\n seek frame_ first_frame_correction_in_secs_ is %.6f, seek_seconds_=%f, position_=%f \n", first_frame_correction_in_secs_, seek_seconds_, position_);
	float targetPosition = seek_seconds_;
	float currentPosition = position_;
	float frameDuration = duration_;
	if(targetPosition < currentPosition){
		this->Destroy();
		this->Init(accompany_file_path_);
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
//		LOGI("currentPosition is %.3f", currentPosition);
		av_free_packet(&packet_);
	}
	seek_resp_ = true;
	seek_req_ = false;
	seek_success_read_frame_success_ = false;
}

int AACAccompanyDecoder::ReadFrame() {
//	LOGI("enter AACAccompanyDecoder::ReadFrame");
	if(seek_req_){
		this->SeekFrame();
	}
	int ret = 1;
	av_init_packet(&packet_);
	int gotframe = 0;
	int readFrameCode = -1;
	while (true) {
		readFrameCode = av_read_frame(format_context_, &packet_);
//		LOGI(" av_read_frame ...", duration_);
		if (readFrameCode >= 0) {
			if (packet_.stream_index == stream_index_) {
				int len = avcodec_decode_audio4(codec_context_, audio_frame_,
						&gotframe, &packet_);
//				LOGI(" avcodec_decode_audio4 ...", duration_);
				if (len < 0) {
					LOGI("decode audio error, skip packet_");
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
						byte *outbuf[2] = { (byte*) swr_buffer_, NULL };
						numFrames = swr_convert(swr_context_, outbuf,
								audio_frame_->nb_samples * ratio,
								(const uint8_t **) audio_frame_->data,
								audio_frame_->nb_samples);
						if (numFrames < 0) {
							LOGI("fail resample audio");
							ret = -1;
							break;
						}
						audioData = swr_buffer_;
					} else {
						if (codec_context_->sample_fmt != AV_SAMPLE_FMT_S16) {
							LOGI("bucheck, audio format is invalid");
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
						actualSeekPosition = position_;
						seek_success_read_frame_success_ = true;
					}
					audio_buffer_size_ = numFrames * numChannels;
//					LOGI(" \n duration_ is %.6f position_ is %.6f audio_buffer_size_ is %d\n", duration_, position_, audio_buffer_size_);
					audio_buffer_ = (short*) audioData;
					audio_buffer_cursor_ = 0;
					break;
				}
			}
		} else {
			ret = -1;
			break;
		}
	}
	av_free_packet(&packet_);
	return ret;
}
void AACAccompanyDecoder::Destroy() {
	if (NULL != swr_buffer_) {
		free(swr_buffer_);
		swr_buffer_ = NULL;
		swr_buffer_size_ = 0;
	}
	if (NULL != swr_context_) {
		swr_free(&swr_context_);
		swr_context_ = NULL;
	}
	if (NULL != audio_frame_) {
		av_free (audio_frame_);
		audio_frame_ = NULL;
	}
	if (NULL != codec_context_) {
		avcodec_close(codec_context_);
		codec_context_ = NULL;
	}
	if (NULL != format_context_) {
		LOGI("leave LiveReceiver::Destroy");
		avformat_close_input(&format_context_);
		format_context_ = NULL;
	}
}
