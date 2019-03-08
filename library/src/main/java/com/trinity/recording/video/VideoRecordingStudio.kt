package com.trinity.recording.video

import android.os.Build
import com.trinity.VideoStudio
import com.trinity.camera.CameraParamSettingException
import com.trinity.camera.PreviewScheduler
import com.trinity.recording.RecordingImplType
import com.trinity.recording.exception.*
import com.trinity.recording.service.PlayerService
import com.trinity.recording.service.factory.PlayerServiceFactory
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl
import com.trinity.recording.video.service.MediaRecorderService
import com.trinity.recording.video.service.factory.MediaRecorderServiceFactory

class VideoRecordingStudio(// 输出video的路径
  private var recordingImplType: RecordingImplType,
  private val onComletionListener: PlayerService.OnCompletionListener,
  private val recordingStudioStateCallback: RecordingStudioStateCallback
) {

  private var playerService: PlayerService? = null

  private var recorderService: MediaRecorderService? = null

  /**
   * 获取录音器的参数
   */
  val recordSampleRate: Int
    get() {
      return recorderService?.sampleRate ?: SAMPLE_RATE_IN_HZ
    }

  val audioBufferSize: Int
    get() {
      // TODO size ?
      return recorderService?.audioBufferSize ?: SAMPLE_RATE_IN_HZ
    }

  interface RecordingStudioStateCallback {
    fun onStartRecordingException(exception: StartRecordingException)
  }

  @Throws(RecordingStudioException::class)
  fun initRecordingResource(scheduler: PreviewScheduler) {
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

  @Synchronized
  private fun destroyRecordingResource() {
    playerService?.stop()
    playerService = null

    recorderService?.stop()
    recorderService?.destoryMediaRecorderProcessor()
    recorderService = null
  }

  // here define use MediaCodec or not
  fun startVideoRecording(
    outputPath: String, bitRate: Int, videoWidth: Int, videoHeight: Int,
    audioSampleRate: Int,
    qualityStrategy: Int, adaptiveBitrateWindowSizeInSecs: Int, adaptiveBitrateEncoderReconfigInterval: Int,
    adaptiveBitrateWarCntThreshold: Int, adaptiveMinimumBitrate: Int,
    adaptiveMaximumBitrate: Int, useHardWareEncoding: Boolean
  ) {

    object : Thread() {
      override fun run() {
        try {
          //这里面不应该是根据Producer是否选用硬件编码器再去看Consumer端, 这里应该对于Consumer端是透明的
          val ret = startConsumer(
            outputPath,
            bitRate,
            videoWidth,
            videoHeight,
            audioSampleRate,
            qualityStrategy,
            adaptiveBitrateWindowSizeInSecs,
            adaptiveBitrateEncoderReconfigInterval,
            adaptiveBitrateWarCntThreshold,
            adaptiveMinimumBitrate,
            adaptiveMaximumBitrate
          )
          if (ret < 0) {
            destroyRecordingResource()
          } else {
            startProducer(videoWidth, videoHeight, bitRate, useHardWareEncoding, qualityStrategy)
          }
        } catch (exception: StartRecordingException) {
          //启动录音失败，需要把资源销毁，并且把消费者线程停止掉
          stopRecording()
          recordingStudioStateCallback?.onStartRecordingException(exception)
        }

      }
    }.start()
  }

  private fun startConsumer(
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
  private fun startProducer(
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
  fun stopRecording() {
    destroyRecordingResource()
    VideoStudio.instance.stopRecord()
  }

  @Throws(CameraParamSettingException::class)
  fun switchCamera() {
    recorderService?.switchCamera()
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

  companion object {
    const val VIDEO_FRAME_RATE = 24
    const val audioSampleRate = 44100
    const val audioChannels = 2
    const val audioBitRate = 64 * 1000
    protected var SAMPLE_RATE_IN_HZ = 44100
  }

}
