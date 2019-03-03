package com.trinity.media;

public class SoundTrackController {
	public interface OnCompletionListener{
		public void onCompletion();
	};
	private OnCompletionListener onCompletionListener;
	public void setOnCompletionListener(OnCompletionListener onCompletionListener){
		this.onCompletionListener = onCompletionListener;
	}
	/**
	 * 设置播放文件地址，有可能是伴唱原唱都要进行设置
	 */
	public native boolean setAudioDataSource(String accompanyPath, float percent);
	/**
	 * 获得伴奏的采样频率
	 */
	public native int getAccompanySampleRate();
	/**
	 * 播放伴奏
	 */
	public native void play();
	/**
	 * 获得播放伴奏的当前时间
	 */
	public native int getCurrentTimeMills();
	/**
	 * 设置播放的音量
	 */
	public native void setVolume(float volume);
	/**
	 * 判断当前播放器播放状态
	 */
	public native boolean isPlaying();
	/**
	 * 如果有伴唱和原唱的时候，进行切换原唱伴唱
	 */
	public native void setMusicSourceFlag(boolean musicSourceFlag);
	/**
	 * 停止伴奏
	 */
	public native void stop();
	
	/**
	 * 暂停伴奏
	 */
	public native void pause();
	
	/**
	 * Seek伴奏
	 */
	public native void seekToPosition(float currentTimeSeconds, float delayTimeSeconds);
	
	public void onCompletion(){
		onCompletionListener.onCompletion();
	}
	
}
