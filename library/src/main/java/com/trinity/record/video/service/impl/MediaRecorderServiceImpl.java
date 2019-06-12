package com.trinity.record.video.service.impl;

import android.content.res.AssetManager;
import com.trinity.camera.CameraParamSettingException;
import com.trinity.camera.PreviewFilterType;
import com.trinity.encoder.MediaCodecSurfaceEncoder;
import com.trinity.record.TrinityRecord;
import com.trinity.record.exception.AudioConfigurationException;
import com.trinity.record.exception.StartRecordingException;
import com.trinity.record.service.RecorderService;
import com.trinity.record.video.service.MediaRecorderService;

public class MediaRecorderServiceImpl implements MediaRecorderService {

	private RecorderService audioRecorderService;
	private TrinityRecord mRecord;

	public MediaRecorderServiceImpl(RecorderService recorderService, TrinityRecord record) {
		this.audioRecorderService = recorderService;
		mRecord = record;
	}

	@Override
	public void switchCamera() throws CameraParamSettingException {
//		mRecord.switchCameraFacing();
	}
	
	@Override
	public void initMetaData() throws AudioConfigurationException {
		audioRecorderService.init();
	}

	@Override
	public boolean initMediaRecorderProcessor() {
		return audioRecorderService.initAudioRecorderProcessor();
	}

	@Override
	public boolean start(int width, int height, int videoBitRate, int frameRate, boolean useHardWareEncoding, int strategy) throws StartRecordingException {
		audioRecorderService.start();
		if(useHardWareEncoding){
			if(MediaCodecSurfaceEncoder.IsInNotSupportedList()){
				useHardWareEncoding = false;
			}
		}
//		previewScheduler.startEncoding(width, height, videoBitRate, frameRate, useHardWareEncoding, strategy);
		return useHardWareEncoding;
	}

	@Override
	public void destoryMediaRecorderProcessor() {
		audioRecorderService.destroyAudioRecorderProcessor();
	}

	@Override
	public void stop() {
		audioRecorderService.stop();
//		previewScheduler.stop();
	}

	@Override
	public int getAudioBufferSize() {
		return audioRecorderService.getAudioBufferSize();
	}

	@Override
	public int getSampleRate() {
		return audioRecorderService.getSampleRate();
	}

	@Override
	public void switchPreviewFilter(AssetManager assetManager, PreviewFilterType filterType) {
	}

	@Override
	public void enableUnAccom() {
		if (audioRecorderService != null) {
			audioRecorderService.enableUnAccom();
		}
	}

}
