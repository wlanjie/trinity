package com.trinity.player;

import android.os.Build;
import android.util.Log;

public abstract class MediaCodecDecoderLifeCycle {

	/** MediaCodec related code **/
	protected MediaCodecDecoder mediaCodecDecoder;
	protected boolean useMediaCodec = false;
	/** 1:Java客户端可以强制使用 软件/硬件 解码器 **/
	public void setUseMediaCodec(boolean value) {
		useMediaCodec = value;
	}
	/** 2:底层判断是否使用硬件解码器 **/
	public boolean isHWCodecAvaliableFromNative() {
		boolean ret = false;
		Log.i("problem", "useMediaCodec " + useMediaCodec);
		if (useMediaCodec) {
			String manufacturer = Build.MANUFACTURER;
			String model = Build.MODEL;
			Log.d("problem", "manufacturer=" + manufacturer + " model=" + model);
			if (MediaCodecDecoder.IsInAndriodHardwareBlacklist()){
				ret = false;
			} else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
				// 4.3
				// MediaCodec under android 4.3+ seems to be very stable and fast
				ret = true;
			} else if (MediaCodecDecoder.IsInAndriodHardwareWhitelist()){
				ret = true;
			}
		}
		return ret;
	}
	/** 3:底层创建硬件解码器,比较重要的是texId以及其他MetaData的传入 **/
	public boolean createVideoDecoderFromNative(int width, int height, int texId, byte[] sps, int spsSize, byte[] pps,
			int ppsSize) {
		mediaCodecDecoder = new MediaCodecDecoder();
		return mediaCodecDecoder.CreateVideoDecoder(width, height, texId, sps, spsSize, pps, ppsSize);
	}

	/** 4-1:当需要解码一个packet(compressedData)的时候调用这个方法 **/
	public int decodeFrameFromNative(byte[] frameData, int inputSize, long timeStamp) {
		return mediaCodecDecoder.DecodeFrame(frameData, inputSize, timeStamp);
	}
	/** 4-2:调用了2-1之后需要调用这方法获取纹理 **/
	public long updateTexImageFromNative() {
		long rec = 0;
		if (mediaCodecDecoder == null){
			return rec;
		} else{
			return mediaCodecDecoder.updateTexImage();
		}
	}
	/** 5:当快进等操作执行的时候需要清空现有的解码器内部的数据 **/
	public void flushMediaCodecBuffersFromNative() {
		if (null != mediaCodecDecoder){
			mediaCodecDecoder.beforeSeek();
		}
	}
	/** 6:销毁解码器 **/
	public void cleanupDecoderFromNative() {
		if (null != mediaCodecDecoder){
			mediaCodecDecoder.CleanupDecoder();
		}
	}
}
