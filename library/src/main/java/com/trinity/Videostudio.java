package com.trinity;

public class Videostudio {

  private static Videostudio instance = new Videostudio();

  private Videostudio() {
  }

  public static Videostudio getInstance() {
    return instance;
  }

  /**
   * 开启普通录制歌曲的视频的后台Publisher线程
   **/
  public native int startCommonVideoRecord(String outputPath, int videoWidth,
                                           int videoheight, int videoFrameRate, int videoBitRate,
                                           int audioSampleRate, int audioChannels, int audioBitRate,
                                           int qualityStrategy,
                                           int adaptiveBitrateWindowSizeInSecs,
                                           int adaptiveBitrateEncoderReconfigInterval,
                                           int adaptiveBitrateWarCntThreshold,
                                           int adaptiveMinimumBitrate,
                                           int adaptiveMaximumBitrate);

  public int startVideoRecord(String outputPath, int videoWidth,
                              int videoheight, int videoFrameRate, int videoBitRate,
                              int audioSampleRate, int audioChannels, int audioBitRate,
                              int qualityStrategy, int adaptiveBitrateWindowSizeInSecs, int adaptiveBitrateEncoderReconfigInterval,
                              int adaptiveBitrateWarCntThreshold, int adaptiveMinimumBitrate,
                              int adaptiveMaximumBitrate) {
    return this.startCommonVideoRecord(outputPath, videoWidth, videoheight, videoFrameRate, videoBitRate, audioSampleRate,
        audioChannels, audioBitRate, qualityStrategy, adaptiveBitrateWindowSizeInSecs, adaptiveBitrateEncoderReconfigInterval,
        adaptiveBitrateWarCntThreshold, adaptiveMinimumBitrate, adaptiveMaximumBitrate);
  }

  /**
   * 停止录制视频
   **/
  public void stopRecord() {
    this.stopVideoRecord();
  }

  public native void stopVideoRecord();

}