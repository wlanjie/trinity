package com.trinity.record.exception;

public class StartRecordingException extends RecordingStudioException {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1450096806486292659L;

	public StartRecordingException() {
		super("启动录音器失败");
	}
}
