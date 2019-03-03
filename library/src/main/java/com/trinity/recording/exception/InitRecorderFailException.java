package com.trinity.recording.exception;

public class InitRecorderFailException extends RecordingStudioException {

	private static final long serialVersionUID = 1204332793566791080L;
	public InitRecorderFailException() {
		super("初始化录音器失败");
	}

}
