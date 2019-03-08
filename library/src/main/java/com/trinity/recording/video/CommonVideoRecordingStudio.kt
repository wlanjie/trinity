package com.trinity.recording.video

import android.os.Build
import android.os.Handler
import com.trinity.VideoStudio
import com.trinity.camera.PreviewScheduler
import com.trinity.recording.RecordingImplType
import com.trinity.recording.exception.*
import com.trinity.recording.service.PlayerService
import com.trinity.recording.service.factory.PlayerServiceFactory
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl
import com.trinity.recording.video.service.factory.MediaRecorderServiceFactory

class CommonVideoRecordingStudio(
  recordingImplType: RecordingImplType,
  private val onComletionListener: PlayerService.OnCompletionListener,
  recordingStudioStateCallback: VideoRecordingStudio.RecordingStudioStateCallback
)
  : VideoRecordingStudio(recordingImplType, recordingStudioStateCallback) {

  val isPlayingAccompany: Boolean
    get() {
      return playerService?.isPlayingAccompany ?: false
    }

  @Throws(RecordingStudioException::class)
  override fun initRecordingResource(scheduler: PreviewScheduler) {
    /**
     * 这里一定要注意顺序，先初始化record在初始化player，因为player中要用到recorder中的samplerateSize
     */
    if (scheduler == null) {
      throw RecordingStudioNullArgumentException("null argument exception in initRecordingResource")
    }

    scheduler.resetStopState()
    recorderService = MediaRecorderServiceFactory.instance.getRecorderService(scheduler, recordingImplType)
    recorderService?.initMetaData()
    if (recorderService?.initMediaRecorderProcessor() == false) {
      throw InitRecorderFailException()
    }
    // 初始化伴奏带额播放器 实例化以及init播放器
    playerService = PlayerServiceFactory.instance.getPlayerService(
      onComletionListener,
      RecordingImplType.ANDROID_PLATFORM
    )
    val result = playerService?.setAudioDataSource(AudioRecordRecorderServiceImpl.SAMPLE_RATE_IN_HZ) ?: false
    if (!result) {
      throw InitPlayerFailException()
    }
  }

  override fun startConsumer(
    outputPath: String, videoBitRate: Int, videoWidth: Int, videoHeight: Int, audioSampleRate: Int,
    qualityStrategy: Int, adaptiveBitrateWindowSizeInSecs: Int, adaptiveBitrateEncoderReconfigInterval: Int,
    adaptiveBitrateWarCntThreshold: Int, adaptiveMinimumBitrate: Int,
    adaptiveMaximumBitrate: Int
  ): Int {
    val quality = ifQualityStrayegyEnable(qualityStrategy)
    return VideoStudio.instance.startVideoRecord(
      outputPath,
      videoWidth, videoHeight, VideoRecordingStudio.VIDEO_FRAME_RATE, videoBitRate,
      audioSampleRate, VideoRecordingStudio.audioChannels, VideoRecordingStudio.audioBitRate,
      quality
    )
  }

  private fun ifQualityStrayegyEnable(qualityStrategy: Int): Int {
    return if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) 0 else qualityStrategy
  }

  @Throws(StartRecordingException::class)
  override fun startProducer(
    videoWidth: Int,
    videoHeight: Int,
    videoBitRate: Int,
    useHardWareEncoding: Boolean,
    strategy: Int
  ): Boolean {
    playerService?.start()
    return recorderService?.start(
      videoWidth,
      videoHeight,
      videoBitRate,
      VideoRecordingStudio.VIDEO_FRAME_RATE,
      useHardWareEncoding,
      strategy
    ) ?: false
  }

  override fun destroyRecordingResource() {
    // 销毁伴奏播放器
    playerService?.stop()
    playerService = null
    super.destroyRecordingResource()
  }

  /**
   * 播放一个新的伴奏
   */
  fun startAccompany(musicPath: String) {
    playerService?.startAccompany(musicPath)
  }

  /**
   * 停止播放
   */
  fun stopAccompany() {
    playerService?.stopAccompany()
  }

  /**
   * 暂停播放
   */
  fun pauseAccompany() {
    playerService?.pauseAccompany()
  }

  /**
   * 继续播放
   */
  fun resumeAccompany() {
    playerService?.resumeAccompany()
  }

  /**
   * 设置伴奏的音量大小
   */
  fun setAccompanyVolume(volume: Float, accompanyMax: Float) {
    playerService?.setVolume(volume, accompanyMax)
  }

}
