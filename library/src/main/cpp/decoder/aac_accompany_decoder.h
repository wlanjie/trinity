#ifndef AAC_ACCOMPANY_DECODER_H
#define AAC_ACCOMPANY_DECODER_H
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifndef UINT64_C
#define UINT64_C(value)__CONCAT(value,ULL)
#endif

#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX	9223372036854775807LL
#endif

#include "common_tools.h"
#include "./accompany_decoder.h"

extern "C" {
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libavutil/avutil.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/common.h"
	#include "libavutil/channel_layout.h"
	#include "libavutil/opt.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/mathematics.h"
	#include "libswscale/swscale.h"
	#include "libswresample/swresample.h"
};

#define OUT_PUT_CHANNELS 2

class AACAccompanyDecoder: public AccompanyDecoder {
private:
	AVFormatContext* format_context_;
	AVCodecContext * codec_context_;
	int stream_index_;
	float time_base_;
	AVFrame *audio_frame_;
	AVPacket packet_;
	char* accompany_file_path_;
	bool seek_success_read_frame_success_;
	int packet_buffer_size_;

	/** 每次解码出来的audioBuffer以及这个audioBuffer的时间戳以及当前类对于这个audioBuffer的操作情况 **/
	short* audio_buffer_;
	float position_;
	int audio_buffer_cursor_;
	int audio_buffer_size_;
	float duration_;
	bool need_first_frame_correct_flag_;
	float first_frame_correction_in_secs_;
	SwrContext *swr_context_;
	void *swr_buffer_;
	int swr_buffer_size_;

	int ReadSamples(short *samples, int size);
	int ReadFrame();
	bool AudioCodecIsSupported();

public:
	AACAccompanyDecoder();
	virtual ~AACAccompanyDecoder();

	//获取采样率以及比特率
	virtual int GetMusicMeta(const char *fileString, int *metaData);
	virtual int GetSampleRate(){
		int sampleRate = -1;
		if(NULL != codec_context_){
			sampleRate = codec_context_->sample_rate;
		}
		return sampleRate;
	};
	//初始化这个decoder，即打开指定的mp3文件
	int Init(const char *fileString);
	void SetPacketBufferSize(int packetBufferSizeParam);
	virtual void Init(const char *fileString, int packetBufferSizeParam);
	virtual LiveAudioPacket* DecodePacket();
	//销毁这个decoder
	virtual void Destroy();

	virtual void SeekFrame();
};
#endif //AAC_ACCOMPANY_DECODER_H
