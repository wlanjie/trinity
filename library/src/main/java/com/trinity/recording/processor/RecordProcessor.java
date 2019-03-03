package com.trinity.recording.processor;
public interface RecordProcessor {
	/**
	 * 初始化AudioQueue里面的每个packet的大小
	 */
	void initAudioBufferSize(int audioSampleRate, int audioBufferSize);
	void destroy();
	/**
	 * 将audioBuffer放入队列中
	 */
	void pushAudioBufferToQueue(short[] audioSamples, int audioSampleSize);
	/**
	 * 把还未放入AudioQueue的buffer全部刷出到queue里面
	 */
	void flushAudioBufferToQueue();

}
