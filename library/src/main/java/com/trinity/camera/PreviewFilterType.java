package com.trinity.camera;

/**
 * @author wlanjie
 */

// TODO delete
public enum PreviewFilterType {
	PREVIEW_COOL(10000, "清凉"), 
	PREVIEW_THIN_FACE(10001, "瘦脸"), 
	PREVIEW_NONE(10002, "无"), 
	PREVIEW_ORIGIN(10003, "自然"), 
	PREVIEW_WHITENING(10004, "白皙");

	private int value;
	private String name;

	PreviewFilterType(int value, String name) {
		this.value = value;
		this.name = name;
	}
	public int getValue() {
		return value;
	}
	public String getName() {
		return name;
	}

	public static PreviewFilterType getPreviewTypeByValue(int value) {
		PreviewFilterType result = PREVIEW_NONE;
		switch (value) {
		case 10000:
			result = PREVIEW_COOL;
			break;
		case 10001:
			result = PREVIEW_THIN_FACE;
			break;
		case 10002:
			result = PREVIEW_NONE;
			break;
		case 10003:
			result = PREVIEW_ORIGIN;
			break;
		case 10004:
			result = PREVIEW_WHITENING;
			break;
		default:
			break;
		}
		return result;
	}
}
