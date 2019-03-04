package com.trinity;

public class VideoStudio {

    private static VideoStudio instance = new VideoStudio();
    private long mHandle;

    private VideoStudio() {
    }

    public static VideoStudio getInstance() {
        return instance;
    }

    private native long create();

    /**
     * 开启普通录制歌曲的视频的后台Publisher线程
     **/
    public native int startCommonVideoRecord(long handle, String outputPath, int videoWidth,
                                             int videoheight, int videoFrameRate, int videoBitRate,
                                             int audioSampleRate, int audioChannels, int audioBitRate,
                                             int qualityStrategy);

    public int startVideoRecord(String outputPath, int videoWidth,
                                int videoheight, int videoFrameRate, int videoBitRate,
                                int audioSampleRate, int audioChannels, int audioBitRate,
                                int qualityStrategy, int adaptiveBitrateWindowSizeInSecs, int adaptiveBitrateEncoderReconfigInterval,
                                int adaptiveBitrateWarCntThreshold, int adaptiveMinimumBitrate,
                                int adaptiveMaximumBitrate) {
        mHandle = create();
        return this.startCommonVideoRecord(mHandle, outputPath, videoWidth, videoheight, videoFrameRate, videoBitRate, audioSampleRate,
                audioChannels, audioBitRate, qualityStrategy);
    }

    /**
     * 停止录制视频
     **/
    public void stopRecord() {
        this.stopVideoRecord(mHandle);
        release(mHandle);
    }

    private native void stopVideoRecord(long handle);

    private native void release(long handle);

}