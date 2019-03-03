package com.trinity.recording.exception;

public class InitConsumerThreadFailException extends RecordingStudioException {

	private static final long serialVersionUID = 1204332793566791080L;
	public InitConsumerThreadFailException() {
		super("连接RTMP服务器失败");
	}

}
