package com.trinity.encoder;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class MediaCodecSurfaceEncoder {
    private static final String TAG = "MediaCodecEncoder";
    private static final boolean VERBOSE = false;
    private static final int MEDIA_CODEC_NOSIE_DELTA = 80*1024;

    public static final String MIME_TYPE = "video/avc"; // H.264 Advanced Video
    // Coding
    private static final int IFRAME_INTERVAL = 1; // sync frame every second

    private Surface mInputSurface;
    private MediaCodec mEncoder;
    private BufferInfo mBufferInfo;
    private long lastPresentationTimeUs = -1;

    private MediaMuxer mMuxer;
    private int mTrackIndex;

    public MediaCodecSurfaceEncoder() {

    }

    public MediaCodecSurfaceEncoder(int width, int height, int bitRate, int frameRate, String outputPath) throws Exception {
    	Log.i("problem", String.format("MediaCodecSurfaceEncoder width:%d, height:%d", width, height));
        MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, width, height);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
        if (VERBOSE)
            Log.d(TAG, "format: " + format);
        try {
            mEncoder = MediaCodec.createEncoderByType(MIME_TYPE);
            mEncoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mInputSurface = mEncoder.createInputSurface();
            mEncoder.start();
        } catch (Exception e) {
            throw e;
        }

        try {
            mMuxer = new MediaMuxer(outputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
        } catch (IOException ioe) {
            throw new RuntimeException("MediaMuxer creation failed", ioe);
        }
        mTrackIndex = -1;
    }

    public MediaCodecSurfaceEncoder(int width, int height, int bitRate, int frameRate) throws Exception {
    	Log.i("problem", String.format("MediaCodecSurfaceEncoder width:%d, height:%d", width, height));
        MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, width, height);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
        if (VERBOSE)
            Log.d(TAG, "format: " + format);
        try {
            mEncoder = MediaCodec.createEncoderByType(MIME_TYPE);
            mEncoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mInputSurface = mEncoder.createInputSurface();
            mEncoder.start();
        } catch (Exception e) {
            throw e;
        }

        mTrackIndex = -1;
    }

    @SuppressLint("NewApi")
	public void hotConfig(int width, int height, int bitRate, int frameRate) throws Exception {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            try {
                if (mEncoder != null) {
                    mEncoder.reset();
                }

                MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, width, height);
                format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
                format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate - MEDIA_CODEC_NOSIE_DELTA);
                format.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
                format.setInteger(MediaFormat.KEY_CAPTURE_RATE, frameRate);
                format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
                mEncoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
                if(null != mInputSurface) {
                    mInputSurface.release();
                }
                mInputSurface = mEncoder.createInputSurface();
                mEncoder.start();

                Log.i(TAG, String.format("HotConfig success bitRate:%d, frameRate:%d", bitRate, frameRate));
            } catch (Exception e) {
                throw e;
            }
        }
    }

    interface MuxerCallback {
        void startMux(MediaFormat mediaFormat);

        void writePacket(ByteBuffer encodedData, BufferInfo bufferInfo);

        void stopMux();
    }

    private MuxerCallback muxerCallback = new MuxerCallback() {
        @Override
        public void startMux(MediaFormat mediaFormat) {
            mTrackIndex = mMuxer.addTrack(mediaFormat);
            mMuxer.start();
        }

        @Override
        public void writePacket(ByteBuffer encodedData, BufferInfo bufferInfo) {
            mMuxer.writeSampleData(mTrackIndex, encodedData, bufferInfo);
        }

        @Override
        public void stopMux() {
            try {
                if (-1 != mTrackIndex && mMuxer != null) {
                    mMuxer.stop();
                    mMuxer.release();
                    mMuxer = null;
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    };

    /**
     * Returns the encoder_'s input surface.
     */
    public Surface getInputSurface() {
        return mInputSurface;
    }

    /**
     * Shuts down the encoder_ thread, and releases encoder_ resources.
     * <p>
     * Does not return until the encoder_ thread has stopped.
     */
    public void shutdown() {
        if (VERBOSE)
            Log.d(TAG, "releasing encoder_ objects");

        try {
            if (null != mEncoder) {
                mEncoder.stop();
                mEncoder.release();
                mEncoder = null;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {
            if (null != muxerCallback) {
                muxerCallback.stopMux();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static final int TIMEOUT_USEC = 5000;

    //4.4系统才有的方法
    @TargetApi(Build.VERSION_CODES.KITKAT)
    public void configureBitRate(int targetBitRate) {
        Bundle bitRateBundle = new Bundle();
        bitRateBundle.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, targetBitRate);
        mEncoder.setParameters(bitRateBundle);
    }

    //TODO:下面这种方式暂时不使用，由于某一些机器MediaCodec的Bug，而是在底层自己destroy然后在重新创建的模式实现ReConfig
    public void reConfigureFromNative(int targetBitRate) {
        Log.i("problem", "reConfigureFromNative : " + targetBitRate);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            configureBitRate(targetBitRate);
        }
    }

    /**
     * Drains all pending output from the decoder, and adds it to the circular
     * buffer.
     *
     */
    public void drainEncoder() {
        mBufferInfo = new BufferInfo();
        ByteBuffer[] encoderOutputBuffers = mEncoder.getOutputBuffers();
        int encoderStatus = mEncoder.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
        if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
            // no output available yet
            Log.i(TAG, "no output available yet");
        } else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
            Log.i(TAG, " not expected for an encoder_");
            encoderOutputBuffers = mEncoder.getOutputBuffers();
        } else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
            MediaFormat mEncodedFormat = mEncoder.getOutputFormat();
            muxerCallback.startMux(mEncodedFormat);
            Log.d(TAG, "encoder_ output format changed: " + mEncodedFormat);
        } else if (encoderStatus < 0) {
            // let's ignore it
            Log.w(TAG, "unexpected result from encoder_.dequeueOutputBuffer: " + encoderStatus);
        } else {
            ByteBuffer encodedData = encoderOutputBuffers[encoderStatus];
            if (encodedData == null) {
                throw new RuntimeException("encoderOutputBuffer " + encoderStatus + " was null");
            }
            if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                // The codec config data was pulled out when we got the
                // INFO_OUTPUT_FORMAT_CHANGED status. The MediaMuxer won't
                // accept
                // a single big blob -- it wants separate csd-0/csd-1 chunks --
                // so simply saving this off won't work.
                if (VERBOSE)
                    Log.d(TAG, "ignoring BUFFER_FLAG_CODEC_CONFIG");
                mBufferInfo.size = 0;
            }
            if (mBufferInfo.presentationTimeUs >= lastPresentationTimeUs) {
                if (mBufferInfo.size != 0) {
                    encodedData.position(mBufferInfo.offset);
                    encodedData.limit(mBufferInfo.offset + mBufferInfo.size);
                    muxerCallback.writePacket(encodedData, mBufferInfo);
                    lastPresentationTimeUs = mBufferInfo.presentationTimeUs;
                    if (VERBOSE) {
                        Log.d(TAG,
                                "sent " + mBufferInfo.size + " bytes to muxer, ts=" + mBufferInfo.presentationTimeUs);
                    }
                } else {
                    Log.i(TAG, "why mBufferInfo.size is equals 0");
                }
            }
            mEncoder.releaseOutputBuffer(encoderStatus, false);

            if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.w(TAG, "reached end of stream unexpectedly");
            }
        }
    }

    public long pullH264StreamFromDrainEncoderFromNative(byte[] returnedData) {
        long val = 0;

        try {
            mBufferInfo = new BufferInfo();
            ByteBuffer[] encoderOutputBuffers = mEncoder.getOutputBuffers();
            int encoderStatus = mEncoder.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
            if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                // no output available yet
                Log.i(TAG, "no output available yet");
            } else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                Log.i(TAG, " not expected for an encoder_");
                encoderOutputBuffers = mEncoder.getOutputBuffers();
            } else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat mEncodedFormat = mEncoder.getOutputFormat();
                Log.d(TAG, "encoder_ output format changed: " + mEncodedFormat);
            } else if (encoderStatus < 0) {
                // let's ignore it
                Log.w(TAG, "unexpected result from encoder_.dequeueOutputBuffer: " + encoderStatus);
            } else {
                ByteBuffer encodedData = encoderOutputBuffers[encoderStatus];
                if (encodedData == null) {
                    throw new RuntimeException("encoderOutputBuffer " + encoderStatus + " was null");
                }
                if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                    // The codec config data was pulled out when we got the
                    // INFO_OUTPUT_FORMAT_CHANGED status. The MediaMuxer won't
                    // accept
                    // a single big blob -- it wants separate csd-0/csd-1 chunks --
                    // so simply saving this off won't work.
                    if (VERBOSE)
                        Log.d(TAG, "ignoring BUFFER_FLAG_CODEC_CONFIG, mBufferInfo.size = " + mBufferInfo.size);

                    if (mBufferInfo.size != 0) {
                        encodedData.position(mBufferInfo.offset);
                        encodedData.limit(mBufferInfo.offset + mBufferInfo.size);

                        val = mBufferInfo.size;

                        encodedData.get(returnedData, 0, mBufferInfo.size);
                    } else {
                        Log.i(TAG, "why mBufferInfo.size is equals 0");
                    }

                    mBufferInfo.size = 0;
                }
                if (mBufferInfo.presentationTimeUs >= lastPresentationTimeUs) {
                    if (mBufferInfo.size != 0) {
                        encodedData.position(mBufferInfo.offset);
                        encodedData.limit(mBufferInfo.offset + mBufferInfo.size);
                        lastPresentationTimeUs = mBufferInfo.presentationTimeUs;
                        val = mBufferInfo.size;

                        encodedData.get(returnedData, 0, mBufferInfo.size);

                        int type = returnedData[4] & 0x1F;
                        if(type == 7) {
                            //某些手机I帧前面会附带上SPS和PPS，剥离掉SPS和PPS，仅使用I帧
                            returnedData = H264ParseUtil.splitIDRFrame(returnedData);
                            val = returnedData.length;
                        }

                        if (VERBOSE) {
                            Log.d(TAG,
                                    "sent " + mBufferInfo.size + " bytes to muxer, ts=" + mBufferInfo.presentationTimeUs);
                        }
                    } else {
                        Log.i(TAG, "why mBufferInfo.size is equals 0");
                    }
                } else {
                    Log.d(TAG, "mBufferInfo.presentationTimeUs < lastPresentationTimeUs");
                }

                mEncoder.releaseOutputBuffer(encoderStatus, false);

                if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    Log.w(TAG, "reached end of stream unexpectedly");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return val;
    }

    public long getLastPresentationTimeUs() {
        return lastPresentationTimeUs;
    }

    // temporary not supported devices list
    public static boolean IsInNotSupportedList() {
//		if(!HWEncoderServerBlackListHelper.isHWEncoderAvailable){
//			return true;
//		}
//		String manufacturer = android.os.Build.MANUFACTURER;
        String model = Build.MODEL;
        if (model.compareTo("Coolpad 8720L") == 0)
            return true;
        if (model.compareTo("vivo X5L") == 0)
            return true;
        if (model.compareTo("CHE-TL00H") == 0)
            return true;
        if (model.indexOf("GT-I9158V") != -1)
            return true;
        if (model.indexOf("HTC D826w") != -1)
            return true;
        if (model.indexOf("y923") != -1)
            return true;
        if (model.indexOf("R7007") != -1)
            return true;
        if (model.indexOf("P6-C00") != -1)
            return true;
        if (model.indexOf("F240L") != -1)    // LG-F240L
            return true;
        if (model.indexOf("A7600") != -1)    // 联想A7600
            return true;

        return false;
    }
}
