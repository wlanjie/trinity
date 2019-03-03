
#include "video_x264_encoder.h"

#define LOG_TAG "VideoX264Encoder"

VideoX264Encoder::VideoX264Encoder(int strategy) {
	this->strategy = strategy;
}

VideoX264Encoder::~VideoX264Encoder() {
}

int VideoX264Encoder::init(int width, int height, int videoBitRate, float frameRate, LiveCommonPacketPool* packetPool) {
	//1:注册所有格式与编解码器
	avcodec_register_all();
	//3:为我们的Container添加Stream(包括编码器)
	if (this->allocVideoStream(width, height, videoBitRate, frameRate) < 0) {
		LOGI("alloc Video Stream Failed... \n");
		return -1;
	}
	//5:分配AVFrame存储编码之前的YUV420P的原始数据
	this->allocAVFrame();
	this->packetPool = packetPool;
	isSPSUnWriteFlag = true;
    return 0;
}

int VideoX264Encoder::encode(LiveVideoPacket * yuy2VideoPacket) {
//	static int long count = 0;
//	static int encodeOneFrameTotal = 0;

	memcpy(yuy2_picture_buf, yuy2VideoPacket->buffer, yuy2VideoPacket->size);
	yuy2_to_yuv420p(video_yuy2_frame->data, video_yuy2_frame->linesize, pCodecCtx->width, pCodecCtx->height, pFrame->data, pFrame->linesize);
	int presentationTimeMills = yuy2VideoPacket->timeMills;
	AVRational time_base = {1, 1000};
	int64_t pts = presentationTimeMills / 1000.0f / av_q2d(time_base);
//	LOGI("Feed Frame presentationTimeMills is %d pts is %llu...", presentationTimeMills, pts);
	pFrame->pts = pts;
	AVPacket pkt = { 0 };
	int got_packet;
	av_init_packet(&pkt);
//	long startEncodeTimeMills = platform_4_live::getCurrentTimeMills();
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_packet);

//	int encodeOneFrame = (int)(platform_4_live::getCurrentTimeMills() - startEncodeTimeMills);
//	LOGI("encode Waste TimeMills is %d", encodeOneFrame);

//	encodeOneFrameTotal += encodeOneFrame;
//	count = count + 1;
//	if (count %10 ==  0){
//		LOGI("encode average time is %d", (int) encodeOneFrameTotal / 10);
//
//		encodeOneFrameTotal = 0;
//	}


	if (ret < 0) {
		LOGI("Error encoding video frame: %s\n", av_err2str(ret));
		ret = -1;
	} else if (got_packet && pkt.size) {
		int presentationTimeMills = pkt.pts * av_q2d(time_base) * 1000.0f;
//		LOGI("pkt : {%llu, %llu}", pkt.pts, pkt.dts);
		int nalu_type = (pkt.data[4] & 0x1F);
		bool isKeyFrame = false;
		byte* frameBuffer;
		int frameBufferSize = 0;
		const char bytesHeader[] = "\x00\x00\x00\x01";
		size_t headerLength = 4; //string literals have implicit trailing '\0'
		if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET == nalu_type) {
			//说明是关键帧
			isKeyFrame = true;
			//分离出sps pps
			vector<NALUnit*>* units = new vector<NALUnit*>();
			parseH264SpsPps(pkt.data, pkt.size, units);
			if (isSPSUnWriteFlag) {
				NALUnit* spsUnit = units->at(0);
				uint8_t* spsFrame = spsUnit->naluBody;
				int spsFrameLen = spsUnit->naluSize;
				NALUnit* ppsUnit = units->at(1);
				uint8_t* ppsFrame = ppsUnit->naluBody;
				int ppsFrameLen = ppsUnit->naluSize;
				//将sps、pps写出去
				int metaBuffertSize = headerLength + spsFrameLen + headerLength + ppsFrameLen;
				byte* metaBuffer = new byte[metaBuffertSize];
				memcpy(metaBuffer, bytesHeader, headerLength);
				memcpy(metaBuffer + headerLength, spsFrame, spsFrameLen);
				memcpy(metaBuffer + headerLength + spsFrameLen, bytesHeader, headerLength);
				memcpy(metaBuffer + headerLength + spsFrameLen + headerLength, ppsFrame, ppsFrameLen);
				this->pushToQueue(metaBuffer, metaBuffertSize, presentationTimeMills, pkt.pts, pkt.dts);
				isSPSUnWriteFlag = false;
			}
			vector<NALUnit*>::iterator i;
			bool isFirstIDRFrame = true;
			for (i = units->begin(); i != units->end(); ++i) {
				NALUnit* unit = *i;
				int naluType = unit->naluType;
				if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET != naluType &&
						H264_NALU_TYPE_PICTURE_PARAMETER_SET != naluType) {
					int idrFrameLen = unit->naluSize;
					frameBufferSize+=headerLength;
					frameBufferSize+=idrFrameLen;
				}
			}
			frameBuffer = new byte[frameBufferSize];
			int frameBufferCursor = 0;
			for (i = units->begin(); i != units->end(); ++i) {
				NALUnit* unit = *i;
				int naluType = unit->naluType;
				if (H264_NALU_TYPE_SEQUENCE_PARAMETER_SET != naluType &&
						H264_NALU_TYPE_PICTURE_PARAMETER_SET != naluType) {
					uint8_t* idrFrame = unit->naluBody;
					int idrFrameLen = unit->naluSize;
					//将关键帧分离出来
					memcpy(frameBuffer + frameBufferCursor, bytesHeader, headerLength);
					frameBufferCursor+=headerLength;
					memcpy(frameBuffer + frameBufferCursor, idrFrame, idrFrameLen);
					frameBufferCursor+=idrFrameLen;
					frameBuffer[frameBufferCursor - idrFrameLen - headerLength] = ((idrFrameLen) >> 24) & 0x00ff;
					frameBuffer[frameBufferCursor - idrFrameLen - headerLength + 1] = ((idrFrameLen) >> 16) & 0x00ff;
					frameBuffer[frameBufferCursor - idrFrameLen - headerLength + 2] = ((idrFrameLen) >> 8) & 0x00ff;
					frameBuffer[frameBufferCursor - idrFrameLen - headerLength + 3] = ((idrFrameLen)) & 0x00ff;
				}
				delete unit;
			}
			delete units;
		} else {
			//说明是非关键帧, 从Packet里面分离出来
			isKeyFrame = false;
			vector<NALUnit*>* units = new vector<NALUnit*>();
			parseH264SpsPps(pkt.data, pkt.size, units);
			vector<NALUnit*>::iterator i;
			for (i = units->begin(); i != units->end(); ++i) {
				NALUnit* unit = *i;
				int nonIDRFrameLen = unit->naluSize;
				frameBufferSize+=headerLength;
				frameBufferSize+=nonIDRFrameLen;
			}
			frameBuffer = new byte[frameBufferSize];
			int frameBufferCursor = 0;
			for (i = units->begin(); i != units->end(); ++i) {
				NALUnit* unit = *i;
				uint8_t* nonIDRFrame = unit->naluBody;
				int nonIDRFrameLen = unit->naluSize;
				memcpy(frameBuffer + frameBufferCursor, bytesHeader, headerLength);
				frameBufferCursor+=headerLength;
				memcpy(frameBuffer + frameBufferCursor, nonIDRFrame, nonIDRFrameLen);
				frameBufferCursor+=nonIDRFrameLen;
				frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength] = ((nonIDRFrameLen) >> 24) & 0x00ff;
				frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 1] = ((nonIDRFrameLen) >> 16) & 0x00ff;
				frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 2] = ((nonIDRFrameLen) >> 8) & 0x00ff;
				frameBuffer[frameBufferCursor - nonIDRFrameLen - headerLength + 3] = ((nonIDRFrameLen)) & 0x00ff;
				delete unit;
			}
			delete units;
		}
		this->pushToQueue(frameBuffer, frameBufferSize, presentationTimeMills, pkt.pts, pkt.dts);
	} else {
		LOGI("No Output Frame...");
	}
	av_free_packet(&pkt);
	return ret;
}

void VideoX264Encoder::pushToQueue(byte* buffer, int size, int timeMills, int64_t pts, int64_t dts) {
	LiveVideoPacket* h264Packet = new LiveVideoPacket();
	h264Packet->buffer = buffer;
	h264Packet->size = size;
	h264Packet->timeMills = timeMills;
//	h264Packet->pts = pts;
//	h264Packet->dts = dts;
	packetPool->pushRecordingVideoPacketToQueue(h264Packet);
}

int VideoX264Encoder::destroy() {
	//Clean
	avcodec_close(pCodecCtx);
	av_free(pFrame);
	av_free(picture_buf);
    return 0;
}

void VideoX264Encoder::reConfigure(int bitRate){
	if (strategy == 1){
		//修正比特率
		int bitRateToSet = bitRate - delta;
		pCodecCtx->rc_max_rate = bitRateToSet;
		pCodecCtx->rc_min_rate = bitRateToSet;
		pCodecCtx->bit_rate = bitRateToSet;
		pCodecCtx->rc_buffer_size = bitRate * 3;
	}else{
		pCodecCtx->rc_max_rate = bitRate + delta;
		pCodecCtx->rc_min_rate = bitRate - delta;
		pCodecCtx->rc_buffer_size = bitRate * 2;
		pCodecCtx->bit_rate = bitRate;
	}

	LOGI("VideoX264Encoder::reConfigure strategy:%d, max_rate:%d, min_rate:%d, bit_rate:%d", strategy, pCodecCtx->rc_max_rate, pCodecCtx->rc_min_rate, pCodecCtx->bit_rate);
}

int VideoX264Encoder::allocVideoStream(int width, int height, int videoBitRate, float frameRate) {
	pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!pCodec) {
		LOGI("Can not find encoder_! \n");
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);
	/* resolution must be a multiple of two */
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	/* frames per second */
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = frameRate;
	/* emit one intra frame every ten frames */
	pCodecCtx->gop_size = (int) frameRate;
//	pCodecCtx->max_b_frames = 3; //TODO:需要我们的pts和dts用编码器的pts和dts，但是有可能会有对不齐的问题，现在只能先把B帧Ignore掉
	pCodecCtx->max_b_frames = 0;

	LOGI("************* gop size is %.2f videoBitRate is %d *************", frameRate, videoBitRate);

	reConfigure(videoBitRate);
	if (strategy == 1){
		//cbr模式设置
		pCodecCtx->rc_initial_buffer_occupancy = pCodecCtx->rc_buffer_size * 3
				/ 4;
		pCodecCtx->rc_buffer_aggressivity = (float) 1.0;
		pCodecCtx->rc_initial_cplx = 0.5;
		pCodecCtx->qcompress = 0;
	}else{
		//vbr模式
		/**  -qscale q  使用固定的视频量化标度(VBR)  以<q>质量为基础的VBR，取值0.01-255，约小质量越好，即qscale 4和-qscale 6，4的质量比6高 。
			 * 					此参数使用次数较多，实际使用时发现，qscale是种固定量化因子，设置qscale之后，前面设置的-b好像就无效了，而是自动调整了比特率。
			 *	 -qmin q 最小视频量化标度(VBR) 设定最小质量，与-qmax（设定最大质量）共用
			 *	 -qmax q 最大视频量化标度(VBR) 使用了该参数，就可以不使用qscale参数  **/
		pCodecCtx->flags |= CODEC_FLAG_QSCALE;
		pCodecCtx->i_quant_factor = 0.8;
		pCodecCtx->qmin = 10;
		pCodecCtx->qmax = 30;
	}

	//H.264
	// 新增语句，设置为编码延迟
	av_opt_set(pCodecCtx->priv_data, "preset", "ultrafast", 0);

	// 实时编码关键看这句，上面那条无所谓
	av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
	av_opt_set(pCodecCtx->priv_data, "profile", "main", 0);
//	av_opt_set(pCodecCtx->priv_data, "crf", "20", AV_OPT_SEARCH_CHILDREN);

//	av_opt_set(pCodecCtx->priv_data, "crf", "30", AV_OPT_SEARCH_CHILDREN);

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGI("Failed to open encoder_! \n");
		return -1;
	}

	return 0;
}

void VideoX264Encoder::allocAVFrame() {
	// target yuv420p buffer
	pFrame = av_frame_alloc();
	int pictureInYUV420PSize = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	picture_buf = (uint8_t *) av_malloc(pictureInYUV420PSize);
	avpicture_fill((AVPicture *) pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	// origin yuy2 buffer
	video_yuy2_frame = av_frame_alloc();
	int pictureInYUY2Size = avpicture_get_size(ORIGIN_COLOR_FORMAT, pCodecCtx->width, pCodecCtx->height);
	yuy2_picture_buf = (uint8_t *) av_malloc(pictureInYUY2Size);
	avpicture_fill((AVPicture *) video_yuy2_frame, yuy2_picture_buf, ORIGIN_COLOR_FORMAT, pCodecCtx->width, pCodecCtx->height);
}
