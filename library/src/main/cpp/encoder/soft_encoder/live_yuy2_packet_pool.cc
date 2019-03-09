#include "./live_yuy2_packet_pool.h"

#define LOG_TAG "LiveYUY2PacketPool"

LiveYUY2PacketPool::LiveYUY2PacketPool() {
	yuy_packet_queue_ = NULL;
}

LiveYUY2PacketPool::~LiveYUY2PacketPool() {
}
//初始化静态成员
LiveYUY2PacketPool* LiveYUY2PacketPool::instance_ = new LiveYUY2PacketPool();
LiveYUY2PacketPool* LiveYUY2PacketPool::GetInstance() {
	return instance_;
}

/************************** YUY2的视频帧 packet queue *******************************************/

void LiveYUY2PacketPool::InitYUY2PacketQueue() {
	if (NULL == yuy_packet_queue_) {
		const char* name = "recording video yuy2 frame_ packet_ queue_";
		yuy_packet_queue_ = new LiveVideoPacketQueue(name);
	}
}

void LiveYUY2PacketPool::AbortYUY2PacketQueue() {
	if (NULL != yuy_packet_queue_) {
		yuy_packet_queue_->Abort();
	}
}

void LiveYUY2PacketPool::DestroyYUY2PacketQueue() {
	if (NULL != yuy_packet_queue_) {
		delete yuy_packet_queue_;
		yuy_packet_queue_ = NULL;
	}
}

int LiveYUY2PacketPool::GetYUY2Packet(LiveVideoPacket **videoPacket, bool block) {
	int result = -1;
	if (NULL != yuy_packet_queue_) {
		result = yuy_packet_queue_->Get(videoPacket, block);
	}
	return result;
}

bool LiveYUY2PacketPool::PushYUY2PacketToQueue(LiveVideoPacket *videoPacket) {
	bool dropFrame = false;
	if (NULL != yuy_packet_queue_) {
		yuy_packet_queue_->Put(videoPacket);
	}
	return dropFrame;
}

int LiveYUY2PacketPool::GetYUY2PacketQueueSize() {
	if (NULL != yuy_packet_queue_) {
		return yuy_packet_queue_->Size();
	}
	return 0;
}

void LiveYUY2PacketPool::ClearYUY2PacketToQueue() {
	if (NULL != yuy_packet_queue_) {
		return yuy_packet_queue_->Flush();
	}
}
/************************** YUY2的视频帧 packet queue *******************************************/
