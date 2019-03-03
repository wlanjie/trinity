package com.trinity.media;
/**
 * 这个是OpenSL 实现的audioRecord 但是目标是
 * (1) 放入queue，并且利用OpenSL的player播放queue里面的东西
 * (2) 写入文件，将pcm流写入文件中
 */
public class AudioRecordEchoController {

	public native void createAudioRecorder(String outPutPath);
	public native void  startRecording();
	public native void  stopRecording();
}
