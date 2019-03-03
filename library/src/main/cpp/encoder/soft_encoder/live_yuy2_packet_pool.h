#ifndef LIVE_YUY2_PACKET_POOL_H
#define LIVE_YUY2_PACKET_POOL_H
#include "livecore/common/video_packet_queue.h"

#define YUY2_PACKET_QUEUE_THRRESHOLD										45

class LiveYUY2PacketPool {
protected:
	LiveYUY2PacketPool(); //注意:构造方法私有
    static LiveYUY2PacketPool* instance; //惟一实例
    /** 边录边合成---视频的YUV数据帧的queue **/
    LiveVideoPacketQueue* yuy2PacketQueue;
public:
    static LiveYUY2PacketPool* GetInstance(); //工厂方法(用来获得实例)
    virtual ~LiveYUY2PacketPool();

    /** 读取出来的YUY2的视频帧数据 **/
    void initYUY2PacketQueue();
    void abortYUY2PacketQueue();
    void destoryYUY2PacketQueue();
    int getYUY2Packet(LiveVideoPacket **videoPacket, bool block);
    bool pushYUY2PacketToQueue(LiveVideoPacket* videoPacket);
    int getYUY2PacketQueueSize();
    void clearYUY2PacketToQueue();
};

#endif //LIVE_YUY2_PACKET_POOL_H
