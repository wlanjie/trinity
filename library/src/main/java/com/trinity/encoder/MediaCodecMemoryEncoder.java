package com.trinity.encoder;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;

import java.nio.ByteBuffer;

//
// this encoder_ uses MediaCodec as encoder_ with memory input
//

@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
public class MediaCodecMemoryEncoder {
	private static final String VIDEO_MIME_TYPE = "video/avc";
	
	private int count;
	private int count1;

	private static final String TAG = "MediaCodecMemoryEncoder";
	private boolean m_verbose = false;
	private MediaCodec m_videoEncoder = null; 	

	private boolean m_inputEOS = false;
	private boolean m_outputEOS = false;
	ByteBuffer[] m_inputBuffers = null;
	ByteBuffer[] m_outputBuffers = null;

	private static final int TIMEOUT_USEC = 5000;
	
	private MediaFormat m_videoFormat;		
	private MediaCodecInfo m_codecInfo = null;

	public MediaCodecMemoryEncoder() {	
		count = 0;
		count1 = 0;
	}

	private static MediaCodecInfo selectCodec(String mimeType) {
		int numCodecs = MediaCodecList.getCodecCount();

		for (int i = 0; i < numCodecs; i++) {
			MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
			if (!codecInfo.isEncoder()) {
				continue;
			}

			String[] types = codecInfo.getSupportedTypes();
			for (int j = 0; j < types.length; j++) {
				if (types[j].equalsIgnoreCase(mimeType)) {
					return codecInfo;
				}
			}
		}
		return null;
	}

	// support these color space currently
	private static boolean isRecognizedFormat(int colorFormat) {
		switch (colorFormat) {
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
			return true;
		default:
			return false;
		}
	}

	// if returned -1, can't find a suitable color format
	public int GetSupportedColorFormat() {
		String manufacturer = Build.MANUFACTURER;
		String model = Build.MODEL;

		if (m_verbose)
			Log.d(TAG, "manufacturer = " + manufacturer + " model = " + model);

		m_codecInfo = selectCodec(VIDEO_MIME_TYPE);

		if ((manufacturer.compareTo("Xiaomi") == 0) // for Xiaomi MI 2SC,
													// selectCodec methord is
													// too slow
				&& (model.compareTo("MI 2SC") == 0))
			return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;

		if ((manufacturer.compareTo("Xiaomi") == 0) // for Xiaomi MI 2,
													// selectCodec methord is
													// too slow
				&& (model.compareTo("MI 2") == 0))
			return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;

		if ((manufacturer.compareTo("samsung") == 0) // for samsung S4,
														// COLOR_FormatYUV420Planar
														// will write green
														// frames
				&& (model.compareTo("GT-I9500") == 0))
			return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;

		if (m_codecInfo == null) {
			Log.e(TAG, "Unable to find an appropriate codec for " + VIDEO_MIME_TYPE);

			return -1;
		}

		try {
			MediaCodecInfo.CodecCapabilities capabilities = m_codecInfo.getCapabilitiesForType(VIDEO_MIME_TYPE);

			for (int i = 0; i < capabilities.colorFormats.length; i++) {
				int colorFormat = capabilities.colorFormats[i];

				if (isRecognizedFormat(colorFormat))
					return colorFormat;
			}
		} catch (Exception e) {
			if (m_verbose)
				Log.d(TAG, "getSupportedColorFormat exception");

			return -1;
		}

		return -1;
	}

	public boolean CreateVideoEncoder(int width, int height, int frameRate, int colorFormat, int iFrameInterval, int bitRate) {
		if (colorFormat == -1)
			return false;
		
		// We avoid the device-specific limitations on width and height by using
		// values
		// that are multiples of 16, which all tested devices seem to be able to
		// handle
		m_videoFormat = MediaFormat.createVideoFormat(VIDEO_MIME_TYPE, width, height);

		// Set some properties. Failing to specify some of these can cause
		// the MediaCodec configure() call to throw an unhelpful exception.
		m_videoFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);
		m_videoFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
		m_videoFormat.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
		m_videoFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, iFrameInterval/frameRate);
		
		if (m_verbose)
			Log.d(TAG, "iFrameInterval is" + iFrameInterval/frameRate);

		if (m_verbose)
			Log.d(TAG, "video format is " + m_videoFormat);

		// Create a MediaCodec for the desired codec, then configure it as
		// an encoder_ with our desired properties.
		try {
			m_videoEncoder = MediaCodec.createByCodecName(m_codecInfo.getName());
		} catch (Exception e) {
			Log.d(TAG, "Failed to create encoder_ for " + VIDEO_MIME_TYPE);
			return false;
		}

		if (m_verbose)
			Log.d(TAG, "MediaCodec for video created");

		try {
			m_videoEncoder.configure(m_videoFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
		} catch (Exception e) {
			Log.d(TAG, "Failed to configure video encoder_");
			return false;
		}

		if (m_verbose)
			Log.d(TAG, "video MediaCodec configured");

		try {
			m_videoEncoder.start();
		} catch (Exception e) {
			Log.d(TAG, "Failed to start video encoder_");
			return false;
		}

		if (m_verbose)
			Log.d(TAG, "video MediaCodec started");

		m_inputBuffers = m_videoEncoder.getInputBuffers();
		m_outputBuffers = m_videoEncoder.getOutputBuffers();

		return true;
	}
	
	public long VideoEncodeFromBuffer(byte[] frameData, long timeStamp, byte[] returnedData) {
		long currentTime = System.currentTimeMillis();
		
		long val = 0;

		MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

		if (m_verbose)
			Log.d(TAG, "video input buffer length: " + frameData.length);

		int encodedSize = 0;

		int inputBufIndex = m_videoEncoder.dequeueInputBuffer(TIMEOUT_USEC);

		if (m_verbose)
			Log.d(TAG, "video inputBufIndex=" + inputBufIndex);

		if (inputBufIndex >= 0) {
			ByteBuffer inputBuf = m_inputBuffers[inputBufIndex];
			inputBuf.clear();
			inputBuf.put(frameData);
			m_videoEncoder.queueInputBuffer(inputBufIndex, 0, frameData.length, timeStamp, 0);
		} else {
			// either all in use, or we timed out during initial setup
			if (m_verbose)
				Log.d(TAG, "video input buffer not available");
		}

		int encoderStatus = m_videoEncoder.dequeueOutputBuffer(info, TIMEOUT_USEC);
		
		if (encoderStatus < 0) {
			if (m_verbose)
				Log.e(TAG, "can't encode frame");
			
			count++;
		}

		if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
			// no output available yet
			if (m_verbose)
				Log.d(TAG, "video no output from encoder_ available");
		} else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
			// not expected for an encoder_
			m_outputBuffers = m_videoEncoder.getOutputBuffers();
			if (m_verbose)
				Log.d(TAG, "video encoder_ output buffers changed");
		} else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
			MediaFormat newFormat = m_videoEncoder.getOutputFormat();

			if (m_verbose)
				Log.d(TAG, "video encoder_ output format changed: " + newFormat);
		} else if (encoderStatus < 0) {
			if (m_verbose)
				Log.d(TAG, "video unexpected result from encoder_.dequeueOutputBuffer: " + encoderStatus);
		} else {
			// encoderStatus >= 0
			ByteBuffer encodedData = m_outputBuffers[encoderStatus];
			if (encodedData == null) {
				Log.e(TAG, "video encoderOutputBuffer " + encoderStatus + " was null");
			}

			// It's usually necessary to adjust the ByteBuffer values to match
			// BufferInfo.
			encodedData.position(info.offset);
			encodedData.limit(info.offset + info.size);
			encodedSize = info.size;

			if (m_verbose)
				Log.d(TAG, "video encoderd size is: " + encodedSize);

			val = encodedSize;

			encodedData.get(returnedData, 0, encodedSize);

			if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
			} else {
				m_outputEOS = (info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
				if (m_verbose)
					Log.d(TAG, (m_outputEOS ? "video (output EOS)" : ""));
			}

			m_videoEncoder.releaseOutputBuffer(encoderStatus, false);
		}
		
		if (m_verbose)
			Log.d("SongStudio VideoRGBMerger", "video encoderd time is: "+(System.currentTimeMillis()-currentTime));
		
		// try to do it again 
		String manufacturer = Build.MANUFACTURER;
		String model = Build.MODEL;
		
		boolean isEncodeAgain = false;
		
		// for this device, can't guarantee every input frame can encode out one packet,
		// has to try encode again for this work around
		if ("HUAWEI".equals(manufacturer) && "HUAWEI MT2-L05".equals(model))
			isEncodeAgain = true;
		
		isEncodeAgain = true;
		
		if (encoderStatus < 0 & count > 3 & isEncodeAgain) {
		    currentTime = System.currentTimeMillis();
			
			val = 0;

		    encodedSize = 0;

			inputBufIndex = m_videoEncoder.dequeueInputBuffer(TIMEOUT_USEC);

			if (m_verbose)
				Log.d(TAG, "video inputBufIndex=" + inputBufIndex);

			if (inputBufIndex >= 0) {
				ByteBuffer inputBuf = m_inputBuffers[inputBufIndex];
				inputBuf.clear();
				inputBuf.put(frameData);
				m_videoEncoder.queueInputBuffer(inputBufIndex, 0, frameData.length, timeStamp, 0);
			} else {
				// either all in use, or we timed out during initial setup
				if (m_verbose)
					Log.d(TAG, "video input buffer not available");
			}

			encoderStatus = m_videoEncoder.dequeueOutputBuffer(info, TIMEOUT_USEC);
			
			if (encoderStatus < 0) {
				if (m_verbose)
					Log.e(TAG, "can't encode frame again");
				
				count1++;
			}

			if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
				// no output available yet
				if (m_verbose)
					Log.d(TAG, "video no output from encoder_ available");
			} else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
				// not expected for an encoder_
				m_outputBuffers = m_videoEncoder.getOutputBuffers();
				if (m_verbose)
					Log.d(TAG, "video encoder_ output buffers changed");
			} else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
				MediaFormat newFormat = m_videoEncoder.getOutputFormat();

				if (m_verbose)
					Log.d(TAG, "video encoder_ output format changed: " + newFormat);
			} else if (encoderStatus < 0) {
				if (m_verbose)
					Log.d(TAG, "video unexpected result from encoder_.dequeueOutputBuffer: " + encoderStatus);
			} else {
				// encoderStatus >= 0
				ByteBuffer encodedData = m_outputBuffers[encoderStatus];
				if (encodedData == null) {
					Log.e(TAG, "video encoderOutputBuffer " + encoderStatus + " was null");
				}

				// It's usually necessary to adjust the ByteBuffer values to match
				// BufferInfo.
				encodedData.position(info.offset);
				encodedData.limit(info.offset + info.size);
				encodedSize = info.size;

				if (m_verbose)
					Log.d(TAG, "video encoderd size is: " + encodedSize);

				val = encodedSize;

				encodedData.get(returnedData, 0, encodedSize);

				if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
				} else {
					m_outputEOS = (info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
					if (m_verbose)
						Log.d(TAG, (m_outputEOS ? "video (output EOS)" : ""));
				}

				m_videoEncoder.releaseOutputBuffer(encoderStatus, false);
			}
		}

		return val;
	}
	
	public void ClearUp() {
		if (m_videoEncoder != null) {
			// m_videoEncoder.flush();
			m_videoEncoder.stop();
			m_videoEncoder.release();
			m_videoEncoder = null;
		}
		
		m_inputEOS = false;
		m_outputEOS = false;	
		
		if (m_verbose) {
			Log.d(TAG, "missing frame count is "+ count);
			Log.d(TAG, "missing frame count again is "+ count1);
		}
	}

	public void NotifyVideoInputEOS() {
		if (m_videoEncoder == null)
			return;

		if (!m_inputEOS) {
			int inputBufIndex = m_videoEncoder.dequeueInputBuffer(TIMEOUT_USEC);

			if (m_verbose)
				Log.d(TAG, "video inputBufIndex=" + inputBufIndex);

			if (inputBufIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
				m_videoEncoder.queueInputBuffer(inputBufIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);

				if (m_verbose)
					Log.d(TAG, "video sent input EOS : m_inputEOS" + m_inputEOS);

				m_inputEOS = true;
			}
		}
	}

	public boolean GetVideoOutputEOS() {
		return m_outputEOS;
	}

	public long FlushVideoEncoderSingleTime(byte[] returnedData) {
		long val = 0;

		if (!m_outputEOS) {
			MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

			int encodedSize = 0;

			int encoderStatus = m_videoEncoder.dequeueOutputBuffer(info, TIMEOUT_USEC);

			if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
				// no output available yet
				if (m_verbose)
					Log.d(TAG, "video no output from encoder_ available");
			} else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
				// not expected for an encoder_
				m_outputBuffers = m_videoEncoder.getOutputBuffers();
				if (m_verbose)
					Log.d(TAG, "video encoder_ output buffers changed");
			} else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
				// not expected for an encoder_
				MediaFormat newFormat = m_videoEncoder.getOutputFormat();

				if (m_verbose)
					Log.d(TAG, "video encoder_ output format changed: " + newFormat);
			} else if (encoderStatus < 0) {
				if (m_verbose)
					Log.d(TAG, "video unexpected result from encoder_.dequeueOutputBuffer: " + encoderStatus);
			} else {
				// encoderStatus >= 0
				ByteBuffer encodedData = m_outputBuffers[encoderStatus];
				if (encodedData == null) {
					Log.e(TAG, "video encoderOutputBuffer " + encoderStatus + " was null");
				}

				// It's usually necessary to adjust the ByteBuffer values to
				// match
				// BufferInfo.
				encodedData.position(info.offset);
				encodedData.limit(info.offset + info.size);
				encodedSize = info.size;

				if (m_verbose)
					Log.d(TAG, "video encoderd size is: " + encodedSize);

				val = encodedSize;

				encodedData.get(returnedData, 0, encodedSize);

				if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
				} else {
					m_outputEOS = (info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
					if (m_verbose)
						Log.d(TAG, (m_outputEOS ? "video (output EOS)" : ""));
				}

				m_videoEncoder.releaseOutputBuffer(encoderStatus, false);
			}
		}

		return val;
	}
	
	// In this device list, MediaCodec recognized input color format AV_PIX_FMT_NV21 as AV_PIX_FMT_NV12,
	public boolean IsInWrongColorFormatList() {
		String manufacturer = Build.MANUFACTURER;
		String model = Build.MODEL;
		if ((manufacturer.compareTo("samsung") == 0) && (model.compareTo("GT-I8552") == 0))
			return true;
		if ((manufacturer.compareTo("samsung") == 0) && (model.compareTo("SCH-I829") == 0))
			return true;
		if ((manufacturer.compareTo("HUAWEI") == 0) && (model.compareTo("HUAWEI MT2-L05") == 0))
			return true;
		return false;
	}
	
	// temporary not supported devices list 
	public static boolean IsInNotSupportedList() {
		String manufacturer = Build.MANUFACTURER;
		String model = Build.MODEL;
		
		// the encoded stream will show artifact during playback 
		if ((manufacturer.compareTo("Xiaomi") == 0) && (model.compareTo("MI 2") == 0))
			return true;
		
		if ((manufacturer.compareTo("Xiaomi") == 0) && (model.indexOf("SC") != -1))
			return true;
		
		if ((manufacturer.compareTo("Xiaomi") == 0) && (model.indexOf("sc") != -1))
			return true;
		
		// the encoder_ can't stop when flushing
		if ((manufacturer.compareTo("samsung") == 0) && (model.compareTo("GT-S7898I") == 0))
			return true;
		
		// the encoder_ size returns a invalid value
		if ((manufacturer.compareTo("HUAWEI") == 0) && (model.compareTo("HUAWEI MT2-L01") == 0))
			return true;
		
		// encode won't get output buffer
		if ((manufacturer.compareTo("samsung") == 0) && (model.compareTo("GT-N7108") == 0))
			return true;
		
		// seek error by MediaPlayer
		if ((manufacturer.compareTo("Xiaomi") == 0) && (model.compareTo("HM NOTE 1TD") == 0))
			return true;
		
		/* umeng black list */
		// TODO: should catch in filter checker, do it future
		if (model.compareTo("SM-G3812") == 0)
			return true;
		
		if (model.compareTo("SM-G3818") == 0)
			return true;
		
		if (model.compareTo("Coolpad 8750") == 0)
			return true;
		
		if (model.compareTo("Galaxy Tab 3 Lite") == 0)
			return true;
		
		if (model.compareTo("GT-T9168") == 0)
			return true;
		
		if (model.compareTo("Droid4X-WIN") == 0)
			return true;
		
		if (model.compareTo("Galaxy Trend 3") == 0)
			return true;
		
		if (model.compareTo("GT-S7278U") == 0)
			return true;
		
		if (model.compareTo("GT-I9118") == 0)
			return true;
		
		if (model.compareTo("GT-S7898") == 0)
			return true;
		
		if (model.compareTo("Lenovo A828t") == 0)
			return true;
		
		if (model.compareTo("Galaxy Mega 5.8") == 0)
			return true;
		
		if (model.compareTo("Coolpad 8295C") == 0)
			return true;
		
		if (model.compareTo("Coolpad8198T") == 0)
			return true;
		
		if (model.compareTo("i8268") == 0)
			return true;
		
		if (model.compareTo("SM-T110") == 0)
			return true;
		
		if (model.compareTo("Coolpad 7060") == 0)
			return true;
		
		if (model.indexOf("X909") != -1)	// OPPO X909
			return true;
		
		if (model.indexOf("U809") != -1)	// 中兴 U809
			return true;
		
		if (model.indexOf("SM-G9250") != -1)	// samsung SM-G9250
			return true;
		
		if (model.indexOf("8720Q") != -1)	// Coolpad 8720Q
			return true;
		
		if (model.indexOf("GT-I8268") != -1)	// samsung GT-I8268
			return true;
		
		if (model.indexOf("GT-N7100") != -1)	// samsung GT-N7100
			return true;
		
		if (model.indexOf("GRAND Neo+") != -1)	// samsung Galaxy GRAND Neo+
			return true;
		
		if (model.indexOf("Galaxy Win") != -1)	// samsung Galaxy Win
			return true;
		
		if (model.indexOf("GT-I9168") != -1)	// samsung GT-I9168
			return true;
		
		if (model.indexOf("Xplay") != -1)	// vivo Xplay
			return true;
		
		if (model.indexOf("NX40X") != -1)	// NX40X
			return true;
		
		if (model.indexOf("I9502") != -1)	// GT-I9502 sumsung S4
			return true;
		
		if (model.indexOf("NX403A") != -1)	// NX403A
			return true;
		
		if (model.indexOf("K900") != -1)	// Lenovo K900
			return true;
		
		if (model.indexOf("N1T") != -1)		// N1T
			return true;
		
		if (model.indexOf("7102") != -1)	// sumsung 7102
			return true;
		
		if (model.indexOf("GT-I9158") != -1)	// samsung GT-I9158
			return true;
		
		if (model.indexOf("7100") != -1)  //sumsung 7100
            return true;
		
		return false;
	}
}
