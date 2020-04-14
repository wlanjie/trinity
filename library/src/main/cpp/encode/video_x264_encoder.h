/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TRINITY_VIDEO_X264_ENCODERE_H
#define TRINITY_VIDEO_X264_ENCODERE_H

#include <vector>
#include "video_packet_queue.h"
#include "h264_util.h"
#include "packet_pool.h"

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
}

#define X264_INPUT_COLOR_FORMAT AV_PIX_FMT_YUV420P
#define ORIGIN_COLOR_FORMAT     AV_PIX_FMT_YUYV422

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
	explicit VideoX264Encoder(int strategy);

	virtual ~VideoX264Encoder();

	int Init(int width, int height, int videoBitRate, float frameRate, PacketPool *packetPool);

	int Encode(VideoPacket *videoPacket);

	void ReConfigure(int bitRate);

	void PushToQueue(uint8_t *buffer, int size, int timeMills, int64_t pts, int64_t dts);

	int Destroy();
};

}  // namespace trinity

#endif  // TRINITY_VIDEO_X264_ENCODERE_H
