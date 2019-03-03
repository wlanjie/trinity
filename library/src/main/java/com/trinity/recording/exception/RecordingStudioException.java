package com.trinity.recording.exception;

public abstract class RecordingStudioException extends Exception {

	private static final long serialVersionUID = -3494098857987918589L;

	public RecordingStudioException() {
		super();
	}

	public RecordingStudioException(String detailMessage) {
		super(detailMessage);
	}

}
