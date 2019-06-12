package com.trinity.record.exception;

public class InitPlayerFailException extends RecordingStudioException {

	private static final long serialVersionUID = 1204332793566791080L;
	public InitPlayerFailException() {
		super("初始化播放器失败");
	}

}
