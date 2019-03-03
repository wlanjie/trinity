package com.trinity.recording;

public enum RecordingImplType {

	ANDROID_PLATFORM("安卓平台提供的API", 0);

	private String name;
	private int value;

	private RecordingImplType(String name, int value) {
		this.name = name;
		this.value = value;
	}

	public String getName() {
		return name;
	}

	public int getValue() {
		return value;
	}

}
