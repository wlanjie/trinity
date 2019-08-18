#ifndef TRINITY_VIDEO_X264_ENCODERE_H
#define TRINITY_VIDEO_X264_ENCODERE_H

#include "video_packet_queue.h"
#include "h264_util.h"
#include "packet_pool.h"
#include <vector>

extern "C" {
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
}

using namespace std;

#define X264_INPUT_COLOR_FORMAT 					AV_PIX_FMT_YUV420P
#define ORIGIN_COLOR_FORMAT     					AV_PIX_FMT_YUYV422

namespace trinity {

class VideoX264Encoder {
private:
	AVCodecContext *codec_context_;
	AVCodec *codec_;
	/** 扔给x264编码器编码的Frame **/
	uint8_t *picture_buf_;
	AVFrame *frame_;
	/** 从预览线程那边扔过来的YUY2数据 **/
	uint8_t *yuy2_picture_buf_;
	AVFrame *video_yuy2_frame_;
	PacketPool *packet_pool_;
	bool sps_unwrite_flag_;
	const int delta_ = 30 * 1000;
	int strategy_ = 0;

	int AllocVideoStream(int width, int height, int videoBitRate, float frameRate);

	void AllocAVFrame();

public:
	VideoX264Encoder(int strategy);

	virtual ~VideoX264Encoder();

	int Init(int width, int height, int videoBitRate, float frameRate, PacketPool *packetPool);

	int Encode(VideoPacket *videoPacket);

	void ReConfigure(int bitRate);

	void PushToQueue(uint8_t *buffer, int size, int timeMills, int64_t pts, int64_t dts);

	int Destroy();
};

}
#endif // TRINITY_VIDEO_X264_ENCODERE_H
