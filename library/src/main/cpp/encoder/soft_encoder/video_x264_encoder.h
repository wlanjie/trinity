#ifndef VIDEO_ENCODER_X264_ENCODER_H
#define VIDEO_ENCODER_X264_ENCODER_H

#include "common_tools.h"
#include "libvideo_consumer/live_common_packet_pool.h"
#include "livecore/common/video_packet_queue.h"
#include "./color_conversion/color_conversion.h"
#include "./h264_util.h"
#include <vector>

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/avcodec.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

using namespace std;

#define X264_INPUT_COLOR_FORMAT 					AV_PIX_FMT_YUV420P
#define ORIGIN_COLOR_FORMAT     					AV_PIX_FMT_YUYV422

class VideoX264Encoder {
private:
	AVCodecContext* codec_context_;
	AVCodec* codec_;
	AVPacket pkt_;
	/** 扔给x264编码器编码的Frame **/
	uint8_t* picture_buf_;
	AVFrame* frame_;
	/** 从预览线程那边扔过来的YUY2数据 **/
	uint8_t* yuy2_picture_buf_;
	AVFrame* video_yuy2_frame_;
	LiveCommonPacketPool* packet_pool_;
	bool sps_unwrite_flag_;
	const int delta_ = 30*1000;
	int frame_rate_;
	int strategy_ = 0;

	int AllocVideoStream(int width, int height, int videoBitRate, float frameRate);
	void AllocAVFrame();
public:
	VideoX264Encoder(int strategy);
    virtual ~VideoX264Encoder();
    int Init(int width, int height, int videoBitRate, float frameRate, LiveCommonPacketPool *packetPool);
    int Encode(LiveVideoPacket *videoPacket);
    void ReConfigure(int bitRate);
    void PushToQueue(byte *buffer, int size, int timeMills, int64_t pts, int64_t dts);
    int Destroy();
};
#endif // VIDEO_ENCODER_X264_ENCODER_H
