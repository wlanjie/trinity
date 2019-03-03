#ifndef SONGSTUDIO_MUSIC_MERGER_H
#define SONGSTUDIO_MUSIC_MERGER_H

#include "CommonTools.h"

class MusicMerger {
protected:

	long frameNum;
	int audioSampleRate;
public:
	MusicMerger();
	~MusicMerger();
	/** 直播中试音以及普通录制歌曲的时候由于不会保存AAC文件，所以只处理音效  现在又有一种情况 是视频也不会编码AAC文件 **/
	virtual void initWithAudioEffectProcessor(int audioSampleRate);
	/** 销毁这个processor **/
	void stop();

	/** ......直播过程中调用到的函数...... **/
	//在直播中调用
	virtual int mixtureMusicBuffer(long frameNum, short* accompanySamples, int accompanySampleSize, short* audioSamples, int audioSampleSize);
};

#endif //SONGSTUDIO_MUSIC_MERGER_H
