#ifndef ACCOMPANY_DECODER_H
#define ACCOMPANY_DECODER_H

#include "common_tools.h"
#include "libvideo_consumer/live_common_packet_pool.h"

#define LOG_TAG "AACAccompanyDecoder"

class AccompanyDecoder {
protected:
	/** 如果使用了快进或者快退命令，则先设置以下参数 **/
	bool seek_req_;
	bool seek_resp_;
	float seek_seconds_;

	float actualSeekPosition;
public:
	AccompanyDecoder(){
		this->seek_seconds_ = 0.0f;
		this->seek_req_ = false;
		this->seek_resp_ = false;
	};
	~AccompanyDecoder(){};
	//获取采样率以及比特率
	virtual int GetMusicMeta(const char *fileString, int *metaData) = 0;
	//初始化这个decoder，即打开指定的mp3文件
	virtual void Init(const char *fileString, int packetBufferSizeParam) = 0;
	virtual int Init(const char *fileString) = 0;
	virtual void SetPacketBufferSize(int packetBufferSizeParam) = 0;
	virtual LiveAudioPacket* DecodePacket() = 0;
	//销毁这个decoder
	virtual void Destroy() = 0;

	virtual int GetSampleRate() = 0;
	void SetSeekReq(bool seekReqParam){
		seek_req_ = seekReqParam;
		if(seek_req_){
			seek_resp_ = false;
		}
	};
	bool HasSeekReq() {
		return seek_req_;
	};
	bool HasSeekResp() {
		return seek_resp_;
	};
	/** 设置到播放到什么位置，单位是秒，但是后边3位小数，其实是精确到毫秒 **/
	void SetPosition(float seconds) {
		actualSeekPosition = -1;
		this->seek_seconds_ = seconds;
		this->seek_req_ = true;
		this->seek_resp_ = false;
	};

	float GetActualSeekPosition(){
		float ret = actualSeekPosition;
		if(ret != -1){
			actualSeekPosition = -1;
		}
		return ret;
	};
	virtual void SeekFrame() = 0;
};
#endif //SONG_DECODER_CONTROLLER_H
