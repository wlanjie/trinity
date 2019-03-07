package com.trinity

class VideoStudio private constructor() {
  private var mHandle: Long = 0

  private external fun create(): Long

  /**
   * 开启普通录制歌曲的视频的后台Publisher线程
   */
  private external fun startCommonVideoRecord(
    handle: Long, outputPath: String, videoWidth: Int,
    videoHeight: Int, videoFrameRate: Int, videoBitRate: Int,
    audioSampleRate: Int, audioChannels: Int, audioBitRate: Int,
    qualityStrategy: Int
  ): Int

  fun startVideoRecord(
    outputPath: String, videoWidth: Int,
    videoHeight: Int, videoFrameRate: Int, videoBitRate: Int,
    audioSampleRate: Int, audioChannels: Int, audioBitRate: Int,
    qualityStrategy: Int ): Int {
    mHandle = create()
    return this.startCommonVideoRecord(
      mHandle, outputPath, videoWidth, videoHeight, videoFrameRate, videoBitRate, audioSampleRate,
      audioChannels, audioBitRate, qualityStrategy
    )
  }

  /**
   * 停止录制视频
   */
  fun stopRecord() {
    this.stopVideoRecord(mHandle)
    release(mHandle)
  }

  private external fun stopVideoRecord(handle: Long)

  private external fun release(handle: Long)

  companion object {
    @JvmStatic
    val instance = VideoStudio()
  }

}