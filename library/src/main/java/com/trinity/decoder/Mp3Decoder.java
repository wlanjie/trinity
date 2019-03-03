package com.trinity.decoder;

public interface Mp3Decoder {

	public void init(float packetBufferTimePercent, int vocalSampleRate);

	public void setAccompanyVolume(float volume, float accompanyMax);
	
	public void destory();

	public int readSamples(short[] samples, int[] slientSizeArr, int[] extraSampleType);

	public void startAccompany(String path);
	public void pauseAccompany();
	public void resumeAccompany();
	public void stopAccompany();
}
