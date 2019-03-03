#ifndef SONGSTUDIO_RESAMPLER_H
#define SONGSTUDIO_RESAMPLER_H
#include "CommonTools.h"
extern "C" {
	#include "libavutil/opt.h"
	#include "libavutil/channel_layout.h"
	#include "libavutil/samplefmt.h"
	#include "libswresample/swresample.h"
};

class Resampler {
private:
	int src_rate;
	int dst_rate;
	int src_nb_samples;
	int64_t src_ch_layout, dst_ch_layout;
	int src_nb_channels, dst_nb_channels;
	int src_linesize, dst_linesize;
	int dst_nb_samples, max_dst_nb_samples;
	uint8_t **dst_data;
	uint8_t **src_data;
	enum AVSampleFormat src_sample_fmt, dst_sample_fmt;
	int dst_bufsize;
	const char *fmt;
	struct SwrContext *swr_ctx;
public:
	Resampler();
	~Resampler();
	//每个channel里面的maxSamples
	virtual int init(int _src_rate = 48000, int _dst_rate = 44100, int _max_src_nb_samples = 1024, int src_channels = 1, int to_channels = 1);
	virtual int process(short *in_data, uint8_t *out_data, int real_src_nb_samples, int* out_nb_bytes);
	virtual int process(short **in_data, uint8_t *out_data, int real_src_nb_samples, int* out_nb_bytes);
	virtual int convert(uint8_t *out_data, int real_src_nb_samples, int* out_nb_bytes);
	virtual void destroy();

	int getSourceSampleRate(){
		return src_rate;
	};
};

#endif //SONGSTUDIO_RESAMPLER_H
