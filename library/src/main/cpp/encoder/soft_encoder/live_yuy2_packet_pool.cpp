#include "./live_yuy2_packet_pool.h"

#define LOG_TAG "LiveYUY2PacketPool"

LiveYUY2PacketPool::LiveYUY2PacketPool() {
	yuy2PacketQueue = NULL;
}

LiveYUY2PacketPool::~LiveYUY2PacketPool() {
}
//初始化静态成员
LiveYUY2PacketPool* LiveYUY2PacketPool::instance = new LiveYUY2PacketPool();
LiveYUY2PacketPool* LiveYUY2PacketPool::GetInstance() {
	return instance;
}

/************************** YUY2的视频帧 packet queue *******************************************/

void LiveYUY2PacketPool::initYUY2PacketQueue() {
	if (NULL == yuy2PacketQueue) {
		const char* name = "recording video yuy2 frame packet queue_";
		yuy2PacketQueue = new LiveVideoPacketQueue(name);
	}
}

void LiveYUY2PacketPool::abortYUY2PacketQueue() {
	if (NULL != yuy2PacketQueue) {
		yuy2PacketQueue->abort();
	}
}

void LiveYUY2PacketPool::destoryYUY2PacketQueue() {
	if (NULL != yuy2PacketQueue) {
		delete yuy2PacketQueue;
		yuy2PacketQueue = NULL;
	}
}

int LiveYUY2PacketPool::getYUY2Packet(LiveVideoPacket **videoPacket, bool block) {
	int result = -1;
	if (NULL != yuy2PacketQueue) {
		result = yuy2PacketQueue->get(videoPacket, block);
	}
	return result;
}

bool LiveYUY2PacketPool::pushYUY2PacketToQueue(LiveVideoPacket* videoPacket) {
	bool dropFrame = false;
	if (NULL != yuy2PacketQueue) {
		yuy2PacketQueue->put(videoPacket);
	}
	return dropFrame;
}

int LiveYUY2PacketPool::getYUY2PacketQueueSize() {
	if (NULL != yuy2PacketQueue) {
		return yuy2PacketQueue->size();
	}
	return 0;
}

void LiveYUY2PacketPool::clearYUY2PacketToQueue() {
	if (NULL != yuy2PacketQueue) {
		return yuy2PacketQueue->flush();
	}
}
/************************** YUY2的视频帧 packet queue *******************************************/
