package com.trinity.recording.processor;

public class AudioRecordProcessor implements RecordProcessor {
	private long handle;

	@Override
	public void initAudioBufferSize(int audioSampleRate, int audioBufferSize) {
		handle = init(audioSampleRate, audioBufferSize);
	}

	@Override
	public void pushAudioBufferToQueue(short[] audioSamples, int audioSampleSize) {
		pushAudioBufferToQueue(handle, audioSamples, audioSampleSize);
	}

	@Override
	public void flushAudioBufferToQueue() {
		flushAudioBufferToQueue(handle);
	}

	private native long init(int audioSampleRate, int audioBufferSize);

	private native void flushAudioBufferToQueue(long handle);
	private native void destroy(long handle);

	private native int pushAudioBufferToQueue(long handle, short[] audioSamples,
			int audioSampleSize);

	@Override
	public void destroy() {
		destroy(handle);
	}

}
