package com.trinity.record.exception;

public class AudioConfigurationException extends RecordingStudioException {
	private static final long serialVersionUID = 491222852782937903L;

	public AudioConfigurationException() {
		super("没有找到合适的音频配置");
	}
}
