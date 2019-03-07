package com.trinity.recording.video

import android.util.Log
import com.trinity.VideoStudio
import com.trinity.camera.CameraParamSettingException
import com.trinity.camera.PreviewScheduler
import com.trinity.recording.RecordingImplType
import com.trinity.recording.exception.InitRecorderFailException
import com.trinity.recording.exception.RecordingStudioException
import com.trinity.recording.exception.StartRecordingException
import com.trinity.recording.service.PlayerService
import com.trinity.recording.video.service.MediaRecorderService
import com.trinity.recording.video.service.factory.MediaRecorderServiceFactory

abstract class VideoRecordingStudio(// 输出video的路径
  protected var recordingImplType: RecordingImplType, recordingStudioStateCallback: RecordingStudioStateCallback
) {

  protected var playerService: PlayerService? = null

  protected var recorderService: MediaRecorderService? = null

  protected var recordingStudioStateCallback: RecordingStudioStateCallback? = recordingStudioStateCallback

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
  open fun initRecordingResource(scheduler: PreviewScheduler) {
    /**
     * 这里一定要注意顺序，先初始化record在初始化player，因为player中要用到recorder中的samplerateSize
     */
    recorderService = MediaRecorderServiceFactory.instance.getRecorderService(scheduler, recordingImplType)
    recorderService?.initMetaData()
    if (recorderService?.initMediaRecorderProcessor() == false) {
      throw InitRecorderFailException()
    }
  }

  @Synchronized
  open fun destroyRecordingResource() {
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
    //设置初始编码率
    if (bitRate > 0) {
      initializeVideoBitRate = bitRate * 1000
      Log.i("problem", "initializeVideoBitRate:$initializeVideoBitRate,useHardWareEncoding:$useHardWareEncoding")
    }

    object : Thread() {
      override fun run() {
        try {
          //这里面不应该是根据Producer是否选用硬件编码器再去看Consumer端, 这里应该对于Consumer端是透明的
          val ret = startConsumer(
            outputPath,
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
            startProducer(videoWidth, videoHeight, useHardWareEncoding, qualityStrategy)
          }
        } catch (exception: StartRecordingException) {
          //启动录音失败，需要把资源销毁，并且把消费者线程停止掉
          stopRecording()
          this@VideoRecordingStudio.recordingStudioStateCallback!!.onStartRecordingException(exception)
        }

      }
    }.start()
  }

  protected abstract fun startConsumer(
    outputPath: String, videoWidth: Int, videoHeight: Int, audioSampleRate: Int,
    qualityStrategy: Int, adaptiveBitrateWindowSizeInSecs: Int, adaptiveBitrateEncoderReconfigInterval: Int,
    adaptiveBitrateWarCntThreshold: Int, adaptiveMinimumBitrate: Int,
    adaptiveMaximumBitrate: Int
  ): Int

  @Throws(StartRecordingException::class)
  protected abstract fun startProducer(
    videoWidth: Int,
    videoHeight: Int,
    useHardWareEncoding: Boolean,
    strategy: Int
  ): Boolean

  fun stopRecording() {
    destroyRecordingResource()
    VideoStudio.instance.stopRecord()
  }

  @Throws(CameraParamSettingException::class)
  fun switchCamera() {
    recorderService?.switchCamera()
  }

  companion object {
    const val COMMON_VIDEO_BIT_RATE = 520 * 1000
    var initializeVideoBitRate = COMMON_VIDEO_BIT_RATE

    const val VIDEO_FRAME_RATE = 24
    const val audioSampleRate = 44100
    const val audioChannels = 2
    const val audioBitRate = 64 * 1000
    protected var SAMPLE_RATE_IN_HZ = 44100
  }

}
