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

#include "CommonTools.h"
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
	AVFormatContext* avFormatContext;
	AVCodecContext * avCodecContext;
	int stream_index;
	float timeBase;
	AVFrame *pAudioFrame;
	AVPacket packet;

	char* accompanyFilePath;

	bool seek_success_read_frame_success;
	int packetBufferSize;

	/** 每次解码出来的audioBuffer以及这个audioBuffer的时间戳以及当前类对于这个audioBuffer的操作情况 **/
	short* audioBuffer;
	float position;
	int audioBufferCursor;
	int audioBufferSize;
	float duration;
	bool isNeedFirstFrameCorrectFlag;
	float firstFrameCorrectionInSecs;

	SwrContext *swrContext;
	void *swrBuffer;
	int swrBufferSize;

	int readSamples(short* samples, int size);
	int readFrame();
	bool audioCodecIsSupported();

public:
	AACAccompanyDecoder();
	virtual ~AACAccompanyDecoder();

	//获取采样率以及比特率
	virtual int getMusicMeta(const char* fileString, int * metaData);
	virtual int getSampleRate(){
		int sampleRate = -1;
		if(NULL != avCodecContext){
			sampleRate = avCodecContext->sample_rate;
		}
		return sampleRate;
	};
	//初始化这个decoder，即打开指定的mp3文件
	int init(const char* fileString);
	void setPacketBufferSize(int packetBufferSizeParam);
	virtual void init(const char* fileString, int packetBufferSizeParam);
	virtual LiveAudioPacket* decodePacket();
	//销毁这个decoder
	virtual void destroy();

	virtual void seek_frame();
};
#endif //AAC_ACCOMPANY_DECODER_H
