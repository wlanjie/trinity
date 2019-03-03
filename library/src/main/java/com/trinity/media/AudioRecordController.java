package com.trinity.media;
/**
 * 这个是OpenSL 实现的audioRecord 但是目标是
 * 写入文件，将pcm流写入文件中
 */
public class AudioRecordController {

	public native void createAudioRecorder(String outPutPath);
	public native void  startRecording();
	public native void  stopRecording();
}
