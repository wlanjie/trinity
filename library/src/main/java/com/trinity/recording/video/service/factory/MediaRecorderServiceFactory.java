package com.trinity.recording.video.service.factory;

import com.trinity.camera.PreviewScheduler;
import com.trinity.recording.RecordingImplType;
import com.trinity.recording.service.RecorderService;
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl;
import com.trinity.recording.video.service.MediaRecorderService;
import com.trinity.recording.video.service.impl.MediaRecorderServiceImpl;

public class MediaRecorderServiceFactory {
	private static MediaRecorderServiceFactory instance = new MediaRecorderServiceFactory();
	private MediaRecorderServiceFactory() {}
	public static MediaRecorderServiceFactory getInstance() {
		return instance;
	}

	public MediaRecorderService getRecorderService(PreviewScheduler scheduler, RecordingImplType recordingImplType) {
		RecorderService recorderService = getAudioRecorderService(recordingImplType);
		MediaRecorderService result = new MediaRecorderServiceImpl(recorderService, scheduler);
		return result;
	}

	protected RecorderService getAudioRecorderService(
			RecordingImplType recordingImplType) {
		RecorderService recorderService = null;
		switch (recordingImplType) {
		case ANDROID_PLATFORM:
			recorderService = AudioRecordRecorderServiceImpl.getInstance();
			break;
		default:
			break;
		}
		return recorderService;
	}

}
