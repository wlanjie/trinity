package com.trinity.record.video.service

import android.content.res.AssetManager
import com.trinity.camera.CameraParamSettingException
import com.trinity.camera.PreviewFilterType
import com.trinity.record.exception.AudioConfigurationException
import com.trinity.record.exception.StartRecordingException

interface MediaRecorderService {

  /**
   * 获得后台处理数据部分的buffer大小
   */
  val audioBufferSize: Int

  val sampleRate: Int

  @Throws(CameraParamSettingException::class)
  fun switchCamera()

  /**
   * 初始化录音器的硬件部分
   */
  @Throws(AudioConfigurationException::class)
  fun initMetaData()

  /**
   * 初始化我们后台处理数据部分(处理音频、处理视频)
   */
  fun initMediaRecorderProcessor(): Boolean

  /**
   * 销毁我们的后台处理数据部分
   */
  fun destoryMediaRecorderProcessor()

  /**
   * 开始录音
   */
  @Throws(StartRecordingException::class)
  fun start(
    width: Int,
    height: Int,
    videoBitRate: Int,
    frameRate: Int,
    useHardWareEncoding: Boolean,
    strategy: Int
  ): Boolean

  /**
   * 结束录音
   */
  fun stop()

  fun switchPreviewFilter(assetManager: AssetManager, filterType: PreviewFilterType)

  fun enableUnAccom()

}
