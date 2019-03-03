package com.trinity.recording.service;

import android.os.Handler;

/**
 * 播放器服务
 */
public interface PlayerService {
	public static final int UPDATE_PLAY_VOICE_PROGRESS = 730;

	interface OnCompletionListener {
		void onCompletion();
	}
	
	interface OnPreparedListener {
		void onPrepared();
	}

	/** 设置Handler 用来更新界面 **/
	public void setHandler(Handler mTimeHandler);

	/** 初始化播放器 **/
	public boolean setAudioDataSource(int vocalSampleRate);

	/** 获得伴唱的采样频率 **/
	public int getAccompanySampleRate();

	/** 是否在播放状态 **/
	public boolean isPlaying();
	/** 是否在播放伴奏的状态 **/
	public boolean isPlayingAccompany();
	
	/** 主播开始发布的时候, 调用播放器的开始播放 **/
	public void start();
	
	/** 主播停止发布的时候, 调用播放器的停止播放 **/
	public void stop();
	
	/** 当前主播开播了多长时间 单位是毫秒 **/
	public int getPlayerCurrentTimeMills();
	
	/** 设置伴奏音量的大小 **/
	public void setVolume(float volume, float accompanyMax);
	
	/** 播放一个伴奏 **/
	public void startAccompany(String path);
	
	/** MiniPlayer暂停播放 **/
	public void pauseAccompany();
	/** MiniPlayer继续播放 **/
	public void resumeAccompany();
	/** MiniPlayer停止播放 **/
	public void stopAccompany();
	/** MiniPlayer获取播放伴奏的时间 单位是毫秒 **/
	public int getPlayedAccompanyTimeMills();
	
}
