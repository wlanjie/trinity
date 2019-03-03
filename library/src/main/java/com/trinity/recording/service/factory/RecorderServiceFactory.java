package com.trinity.recording.service.factory;


import com.trinity.recording.RecordingImplType;
import com.trinity.recording.service.RecorderService;
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl;

public class RecorderServiceFactory {
	private static RecorderServiceFactory instance = new RecorderServiceFactory();
	private RecorderServiceFactory(){};
	public static RecorderServiceFactory getInstance(){
		return instance;
	}
	
	public RecorderService getRecorderService(RecordingImplType recordingImplType){
		RecorderService result = null;
		switch (recordingImplType) {
		case ANDROID_PLATFORM:
			result = AudioRecordRecorderServiceImpl.getInstance();
			break;
		default:
			break;
		}
		return result;
	}
}
