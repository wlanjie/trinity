package com.trinity;


public enum PacketBufferTimeEnum {
	TWENTY_PERCENT(0, 0.2f), FIFTY_PERCENT(1, 0.5f), HUNDRED_PERCENT(2, 1.0f), TWO_HUNDRED_PERCENT(
			3, 2.0f), THREE_HUNDRED_PERCENT(4, 3.0f), ERROR_STATE(5, 1.0f);
	private int index;
	private float percent;

	private PacketBufferTimeEnum(int echo, float percent) {
		this.index = echo;
		this.percent = percent;
	}

	public int getIndex() {
		return index;
	}

	public float getPercent() {
		return percent;
	}

	public PacketBufferTimeEnum Upgrade() {
		PacketBufferTimeEnum result = this;
		switch (this) {
		case TWENTY_PERCENT:
			result = FIFTY_PERCENT;
			break;
		case FIFTY_PERCENT:
			result = HUNDRED_PERCENT;
			break;
		case HUNDRED_PERCENT:
			result = TWO_HUNDRED_PERCENT;
			break;
		case TWO_HUNDRED_PERCENT:
			result = THREE_HUNDRED_PERCENT;
			break;
		case THREE_HUNDRED_PERCENT:
			result = ERROR_STATE;
			break;
		default:
			result = ERROR_STATE;
			break;
		}
		return result;
	}

}
