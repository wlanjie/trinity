/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package com.trinity.record

import android.content.Context
import android.graphics.PointF
import android.graphics.SurfaceTexture
import android.provider.Settings
import android.provider.Settings.SettingNotFoundException
import android.view.OrientationEventListener
import android.view.Surface
import com.tencent.mars.xlog.Log
import com.trinity.Constants
import com.trinity.ErrorCode
import com.trinity.OnRecordingListener
import com.trinity.camera.*
import com.trinity.core.Frame
import com.trinity.core.MusicInfo
import com.trinity.core.RenderType
import com.trinity.encoder.MediaCodecSurfaceEncoder
import com.trinity.face.FaceDetection
import com.trinity.face.FaceDetectionImageType
import com.trinity.face.FaceDetectionReport
import com.trinity.face.FlipType
import com.trinity.listener.OnRenderListener
import com.trinity.player.AudioPlayer
import com.trinity.record.exception.AudioConfigurationException
import com.trinity.record.exception.InitRecorderFailException
import com.trinity.record.service.PlayerService
import com.trinity.record.service.RecorderService
import com.trinity.record.service.impl.AudioRecordRecorderServiceImpl
import com.trinity.util.Timer
import java.io.File

/**
 * Create by wlanjie on 2019/3/14-下午8:45
 */
class TrinityRecord(
  val context: Context,
  preview: TrinityPreviewView
) : TrinityPreviewView.PreviewViewCallback, Timer.OnTimerListener, CameraCallback {

  // c++对象指针地址
  private var mHandle: Long = 0
  // 相机对象
  private var mCamera = Camera1(preview.context, this)
  // 预览宽
  private var mCameraWidth = 0
  // 预览高
  private var mCameraHeight = 0
  private var mViewWidth = 0
  private var mViewHeight = 0
  private var mFrame = Frame.CROP
  private var mCameraCallback: CameraCallback ?= null
  // 渲染类型
  private var mRenderType = RenderType.CROP
  // 请求打开摄像头的宽和高
  private var mResolution = PreviewResolution.RESOLUTION_1280x720
  // 预览比例
  private var mAspectRatio = AspectRatio.of(16, 9)
  // surface texture
  private var mSurfaceTexture: SurfaceTexture ?= null
  // texture矩阵
  private val mTextureMatrix = FloatArray(16)
  // 硬编码对象
  private var mSurfaceEncoder = MediaCodecSurfaceEncoder()
  // Surface 对象
  private var mSurface: Surface ?= null
  // 是否第一次创建
  // 第一次创建时创建OpenGL环境,和开启OpenGL线程
  // 第二次时直接创建EGLSurface
  private var mFirst = true
  // Surface是否存在
  // 在createSurface中设置为true
  // 在destroySurface中设置为false
  private var mSurfaceExist = false
  // 是否停止
  private var mStop = false
  // 音频录制
  private val mAudioRecordService: RecorderService = AudioRecordRecorderServiceImpl.instance
  // 音乐播放器
  private var mPlayerService: PlayerService ?= null
  // 计时器
  private var mTimer = Timer(this, 20)
  // 音乐是否播放中
  // 这个标志为true时,在开始录制时,继续播放
  private var mMusicPlaying = false
  // texture回调
  // 可以做特效处理 textureId是OES类型
  private var mOnRenderListener: OnRenderListener ?= null
  // 录制事件
  private var mOnRecordingListener: OnRecordingListener ?= null
  // 是否录制中
  private var mRecording = false
  // 音乐播放器
  private var mAudioPlayer: AudioPlayer ?= null
  // 音乐信息对象
  private var mMusicInfo: MusicInfo ?= null
  // 录制速度
  private var mSpeed = Speed.NORMAL
  // 是否请求打开摄像头
  // 如果textureId还没创建好时设置为true
  // 在textureId创建成功时,打开摄像头
  private var mRequestPreview = false
  // 人脸检测实例
  private var mFaceDetection: FaceDetection ?= null
  // 设备角度检测
  private val mOrientationListener: OrientationEventListener
  // 设备角度
  private var mRotateDegree = 0
  // 设备是否自动旋转
  private var mAutoRotate = false
  private var mFaceDetectionReports: Array<FaceDetectionReport> ?= null

  init {
    preview.setCallback(this)
    mOrientationListener = object: OrientationEventListener(context) {
      override fun onOrientationChanged(orientation: Int) {
        if (orientation == ORIENTATION_UNKNOWN) {
          return
        }
        val o = (orientation + 45) / 90 * 90
        try {
          mAutoRotate = 1 == Settings.System.getInt(context.contentResolver, Settings.System.ACCELEROMETER_ROTATION)
        } catch (e: SettingNotFoundException) {
          e.printStackTrace()
        }
        if (mAutoRotate && o % 360 == 180) {
          return
        }
        mRotateDegree = o % 360
      }
    }
    if (mOrientationListener.canDetectOrientation()) {
      mOrientationListener.enable()
    }
  }

  fun setCameraCallback(callback: CameraCallback) {
    mCameraCallback = callback
  }

  private fun resetStopState() {
    mStop = false
  }

  // SurfaceView surfaceCreated callback
  override fun createSurface(surface: Surface, width: Int, height: Int) {
    startPreview(surface, width, height)
  }

  // SurfaceView surfaceChanged 回调
  override fun resetRenderSize(width: Int, height: Int) {
    mViewWidth = width
    mViewHeight = height
    // 调整绘制区域大小
    setRenderSize(mHandle, width, height)
  }

  // SurfaceView surfaceDestroyed 回调
  override fun destroySurface() {
    Log.i(Constants.TAG, "destroySurface")
    destroyEGLContext()
    mSurfaceExist = false
  }

  private fun startPreview(surface: Surface, width: Int, height: Int) {
    if (mFirst) {
      // 第一次创建c++对象
      mHandle = create()
      // 初始化OpenGL上下文,并开启线程
      prepareEGLContext(mHandle, surface, width, height)
      setRenderType(mRenderType)
      setFrame(mFrame)
      Log.d("tag", "startPreview")
      mFirst = false
    } else {
      // 直接创建EGLSurface
      createWindowSurface(mHandle, surface)
    }
    mSurfaceExist = true
  }

  private fun destroyEGLContext() {
    // 释放OpenGL上下文, 停止线程
    destroyEGLContext(mHandle)
    // 删除c++对象
    release(mHandle)
    mHandle = 0
    mFirst = true
    mSurfaceExist = false
    mStop = false
  }

  /**
   * 设置预览显示类型
   * @param renderType FIX_XY 显示整个屏幕,同时裁剪显示内容, CROP按原比例显示,同时上下留黑
   */
  fun setRenderType(renderType: RenderType) {
    mRenderType = renderType
    setRenderType(mHandle, renderType.ordinal)
  }

  /**
   * 设置播放速度,在录制过程中设置无效
   * 注意: 快速录制时,需要进行丢帧处理,否则录制出来的视频在快4倍速时能达到120fps
   * @param speed Speed
   */
  fun setSpeed(speed: Speed) {
    mSpeed = speed
  }

  /**
   * 设置画幅
   * 目前画幅有: 垂直 横向 1:1方形
   */
  fun setFrame(frame: Frame) {
    mFrame = frame
    setFrame(mHandle, frame.ordinal)
  }

  /**
   * 设置录制回调
   */
  fun setOnRecordingListener(l: OnRecordingListener) {
    mOnRecordingListener = l
  }

  /**
   * 设置Render回调
   */
  fun setOnRenderListener(l: OnRenderListener) {
    mOnRenderListener = l
  }

  /**
   * 设置背景音乐路径
   * @return 0 成功
   */
  fun setBackgroundMusic(info: MusicInfo): Int {
    val file = File(info.path)
    if (!file.exists()) {
      return ErrorCode.FILE_NOT_EXISTS
    }
    mMusicPlaying = false
    mAudioPlayer?.stop()
    mAudioPlayer?.release()
    mAudioPlayer = AudioPlayer()
    mMusicInfo = info
    return ErrorCode.SUCCESS
  }

  /**
   * 开启摄像头预览
   * @param resolution PreviewResolution 预览分辨率选择
   */
  fun startPreview(resolution: PreviewResolution) {
    mResolution = resolution
    if (mSurfaceTexture == null) {
      // opengl环境还没准备好,此时不能打开相机
      // 因为相机打开需要OES的textureId
      mRequestPreview = true
      return
    }
    mSurfaceTexture?.let {
      mCamera.setAspectRatio(mAspectRatio)
      mCamera.start(it, mResolution)
    }
  }

  /**
   * 停止预览, 释放摄像头
   */
  fun stopPreview() {
    mCamera.stop()
    mRequestPreview = false
  }

  /**
   * 切换摄像头
   */
  fun switchCamera() {
    mCamera.setAspectRatio(mAspectRatio)
    val facing = if (getCameraFacing() == Facing.BACK) Facing.FRONT else Facing.BACK
    mCamera.setFacing(facing)
    switchCamera(mHandle)
  }

  /**
   * 设置当前摄像头id
   */
  @Suppress("unused")
  fun setCameraFacing(facing: Facing) {
    mCamera.setFacing(facing)
  }

  /**
   * 获取当前摄像头id
   * @return Facing 返回前后摄像头的枚举定义
   */
  @Suppress("unused")
  fun getCameraFacing(): Facing {
    return mCamera.getFacing()
  }

  /**
   * 开头闪光灯
   * @param flash
   * Flash.OFF 关闭
   * Flash.ON 开启
   * Flash.TORCH
   * Flash.AUTO 自动
   */
  fun flash(flash: Flash) {
    mCamera.setFlash(flash)
  }

  /**
   * 设置camera缩放
   * 100 为最大缩放
   * @param zoom camera缩放的值, 最大100, 最小0
   */
  fun setZoom(zoom: Int) {
    mCamera.setZoom(zoom)
  }

  /**
   * 设置camera曝光度
   * 100 为最大
   * @param exposureCompensation camera曝光的值, 最在100, 最小0
   */
  @Suppress("unused")
  fun setExposureCompensation(exposureCompensation: Int) {
    mCamera.setExposureCompensation(exposureCompensation)
  }

  /**
   * 设置聚焦区域
   * @param point 聚焦区域的x, y值
   */
  fun focus(point: PointF) {
    mCamera.focus(mViewWidth, mViewHeight, point)
  }

  /**
   * 设置人脸检测接口
   * @param faceDetection FaceDetection
   */
  fun setFaceDetection(faceDetection: FaceDetection) {
    faceDetection.releaseDetection()
    faceDetection.createFaceDetection(context, 0)
    mFaceDetection = faceDetection
  }

  override fun dispatchOnFocusStart(where: PointF) {
    mCameraCallback?.dispatchOnFocusStart(where)
  }

  override fun dispatchOnFocusEnd(success: Boolean, where: PointF) {
    mCameraCallback?.dispatchOnFocusEnd(success, where)
  }

  override fun dispatchOnPreviewCallback(data: ByteArray, width: Int, height: Int, orientation: Int) {
    mCameraCallback?.dispatchOnPreviewCallback(data, width, height, orientation)
    if (mFaceDetection == null) {
      onFrameAvailable(mHandle)
      return
    }
    val frontCamera = mCamera.getFacing() == Facing.FRONT
    val inAngle = if (frontCamera) (orientation + 360 - mRotateDegree) % 360 else (orientation + mRotateDegree) % 360
    val outAngle = if (!mAutoRotate) {
      if (frontCamera) (360 - mRotateDegree) % 360 else mRotateDegree % 360
    } else {
      0
    }
    val flipType = if (frontCamera) FlipType.FLIP_Y else FlipType.FLIP_NONE
    val result = mFaceDetection?.faceDetection(data, width, height, FaceDetectionImageType.YUV_NV21, inAngle, outAngle, flipType)
    mFaceDetectionReports = result
    onFrameAvailable(mHandle)
  }

  override fun update(timer: Timer, elapsedTime: Int) {
    mOnRecordingListener?.onRecording(elapsedTime, timer.duration)
  }

  /**
   * 录制结束
   */
  override fun end(timer: Timer) {
    stopRecording()
  }

  @Suppress("unused")
  private fun isLandscape(displayOrientation: Int): Boolean {
    return false
  }


  /**
   * 添加滤镜
   * 如: content.json的绝对路径为 /sdcard/Android/com.trinity.sample/cache/filters/config.json
   * 传入的路径只需要 /sdcard/Android/com.trinity.sample/cache/filters 即可
   * 如果当前路径不包含config.json则添加失败
   * 具体json内容如下:
   * {
   *  "type": 0,
   *  "intensity": 1.0,
   *  "lut": "lut_124/lut_124.png"
   * }
   *
   * json 参数解释:
   * type: 保留字段, 目前暂无作用
   * intensity: 滤镜透明度, 0.0时和摄像头采集的无差别
   * lut: 滤镜颜色表的地址, 必须为本地地址, 而且为相对路径
   *      sdk内部会进行路径拼接
   * @param configPath 滤镜config.json的父目录
   * @return 返回当前滤镜的唯一id
   */
  @Suppress("unused")
  fun addFilter(configPath: String): Int {
    return addFilter(mHandle, configPath)
  }

  /**
   * 更新滤镜
   * @param configPath config.json的路径, 目前对称addFilter说明
   * @param startTime 滤镜的开始时间
   * @param endTime 滤镜的结束时间
   * @param actionId 需要更新哪个滤镜, 必须为addFilter返回的actionId
   */
  @Suppress("unused")
  fun updateFilter(configPath: String, startTime: Int, endTime: Int, actionId: Int) {
    updateFilter(mHandle, configPath, startTime, endTime, actionId)
  }

  /**
   * 删除滤镜
   * @param actionId 需要删除哪个滤镜, 必须为addFilter时返回的actionId
   */
  fun deleteFilter(actionId: Int) {
    deleteFilter(mHandle, actionId)
  }

  /**
   * 添加特效
   * 如: content.json的绝对路径为 /sdcard/Android/com.trinity.sample/cache/effects/config.json
   * 传入的路径只需要 /sdcard/Android/com.trinity.sample/cache/effects 即可
   * 如果当前路径不包含config.json则添加失败
   * @param configPath 滤镜config.json的父目录
   * @return 返回当前特效的唯一id
   */
  fun addAction(configPath: String): Int {
    if (mHandle <= 0) {
      return -1
    }
    return addAction(mHandle, configPath)
  }

  private external fun addAction(handle: Long, config: String): Int

  /**
   * 更新指定特效
   * @param startTime 特效的开始时间
   * @param endTime 特效的结束时间
   * @param actionId 需要更新哪个特效, 必须为addAction返回的actionId
   */
  fun updateActionTime(startTime: Int, endTime: Int, actionId: Int) {
    if (mHandle <= 0) {
      return
    }
    updateActionTime(mHandle, startTime, endTime, actionId)
  }

  private external fun updateActionTime(handle: Long, startTime: Int, endTime: Int, actionId: Int)

  /**
   * 更新指定特效的参数
   * @param actionId Int 需要更新哪个特效, 必须为addAction返回的actionId
   * @param effectName String 需要更新特效的名字, 这个是在json中定好的
   * @param paramName String 更新OpenGL shader中的参数值
   * @param value Float 具体的参数值 0.0 ~ 1.0
   */
  fun updateActionParam(actionId: Int, effectName: String, paramName: String, value: Float) {
    updateActionParam(mHandle, actionId, effectName, paramName, value)
  }

  private external fun updateActionParam(handle: Long, actionId: Int, effectName: String, paramName: String, value: Float)

  /**
   * 删除一个特效
   * @param actionId 需要删除哪个特效, 必须为addAction返回的actionId
   */
  fun deleteAction(actionId: Int) {
    if (mHandle <= 0) {
      return
    }
    deleteAction(mHandle, actionId)
  }

  private external fun deleteAction(handle: Long, actionId: Int)


  /**
   * 设置录制角度
   * @param rotation 角度包含 0 90 180 270
   */
  @Suppress("unused")
  fun setRecordRotation(rotation: Int) {

  }

  /**
   * 设置静音录制
   * 静音录制需要把麦克风采集到的数据全部设置为0即可
   * @param mute true为静音
   */
  @Suppress("unused")
  fun setMute(mute: Boolean) {

  }

  /**
   * 开始录制一段视频
   * @param path 录制的视频保存的地址
   * @param width 录制视频的宽, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
   * @param height 录制视频的高, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
   * @param videoBitRate 视频输出的码率, 如果设置的是2000, 则为2M, 最终输出的和设置的可能有差别
   * @param frameRate 视频输出的帧率
   * @param useHardWareEncode 是否使用硬编码, 如果设置为true, 而硬编码不支持,则自动切换到软编码
   * @param audioSampleRate 音频的采样率
   * @param audioChannel 音频的声道数
   * @param audioBitRate 音频的码率
   * @param duration 需要录制多少时间
   * @return Int ErrorCode.SUCCESS 为成功,其它为失败
   * @throws InitRecorderFailException
   */
  @Throws(InitRecorderFailException::class)
  fun startRecording(path: String,
                  width: Int, height: Int,
                  videoBitRate: Int, frameRate: Int,
                  useHardWareEncode: Boolean,
                  audioSampleRate: Int,
                  audioChannel: Int,
                  audioBitRate: Int,
                  duration: Int): Int {
    synchronized(this) {
      if (mRecording) {
        return ErrorCode.RECORDING
      }
      mTimer.start((duration / mSpeed.value).toInt())
      // TODO mAudioRecordService.sampleRate
      resetStopState()
      try {
        mAudioRecordService.init(mSpeed.value)
      } catch (e: AudioConfigurationException) {
        e.printStackTrace()
        return -1
      }

      mMusicInfo?.let {
        if (mMusicPlaying) {
          mAudioPlayer?.resume()
        } else {
          mMusicPlaying = true
          mAudioPlayer?.start(it.path)
        }
      }

      mAudioRecordService.start()
      setSpeed(mHandle, 1.0f / mSpeed.value)
      startEncode(
        mHandle, path, width, height, videoBitRate, frameRate,
        useHardWareEncode,
        audioSampleRate, audioChannel, audioBitRate
      )
      mRecording = true
    }
    return ErrorCode.SUCCESS
  }

  /**
   * 停止录制
   */
  fun stopRecording() {
    synchronized(this) {
      Log.i("trinity", "stopEncode")
      if (!mRecording) {
        return
      }
      mRecording = false
      mTimer.stop()
      mAudioPlayer?.pause()
      mPlayerService?.pauseAccompany()
      mPlayerService = null
      mAudioRecordService.stop()
      mAudioRecordService.destroyAudioRecorderProcessor()
      stopEncode(mHandle)
    }
  }

  /**
   * 在activity onDestroy方法中调用,释放其它资源
   */
  fun release() {
    synchronized(this) {
      mAudioPlayer?.stop()
      mAudioPlayer?.release()
      mTimer.release()
      mPlayerService?.stopAccompany()
      mPlayerService?.stop()
      mOrientationListener.disable()
      mFaceDetection?.releaseDetection()
    }
  }

  /**
   * 获取人脸信息, 由c++调用
   * @return Array<FaceDetectionReport>? 人脸信息集合
   */
  @Suppress("unused")
  private fun getFaceDetectionReports() = mFaceDetectionReports

  /**
   * 获取预览宽，由c++调用，创建framebuffer
   */
  @Suppress("unused")
  private fun getCameraWidth(): Int {
    if (mSurfaceTexture == null) {
      return 0
    }
    return mCamera.getWidth()
  }

  /**
   * 获取预览宽，由c++调用，创建framebuffer
   */
  @Suppress("unused")
  private fun getCameraHeight(): Int {
    if (mSurfaceTexture == null) {
      return 0
    }
    return mCamera.getHeight()
  }

  /**
   * 由c++回调回来
   * 开启预览
   * @param textureId OES类型的id
   */
  @Suppress("unused")
  private fun startPreviewFromNative(textureId: Int) {
    Log.i(Constants.TAG, "enter startPreviewFromNative")
    mSurfaceTexture = SurfaceTexture(textureId)
    mSurfaceTexture?.let {
//      it.setOnFrameAvailableListener {
//        onFrameAvailable(mHandle)
//      }
      if (mRequestPreview) {
        mRequestPreview = false
        mCamera.setAspectRatio(mAspectRatio)
        mCamera.start(it, mResolution)
      }
    }
    Log.i(Constants.TAG, "leave startPreviewFromNative")
  }

  /**
   * 由c++回调回来
   * 刷新相机采集的内容
   */
  @Suppress("unused")
  private fun updateTexImageFromNative() {
    mSurfaceTexture?.updateTexImage()
  }

  /**
   * surface创建回调
   * 由c++回调
   */
  @Suppress("unused")
  private fun onSurfaceCreated() {
    mOnRenderListener?.onSurfaceCreated()
  }

  /**
   * texture回调
   * 可以做特效处理, textureId为TEXTURE_2D类型
   * @param textureId 相机textureId
   * @param width texture宽
   * @param height texture高
   * @return 返回处理过的textureId, 必须为TEXTURE_2D类型
   */
  @Suppress("unused")
  private fun onDrawFrame(textureId: Int, width: Int, height: Int): Int {
    return mOnRenderListener?.onDrawFrame(textureId, width, height, mTextureMatrix) ?: -1
  }

  /**
   * surface销毁回调
   * 由c++回调
   */
  @Suppress("unused")
  private fun onSurfaceDestroy() {
    mOnRenderListener?.onSurfaceDestroy()
  }

  /**
   * 由c++调用
   * 获取Texture矩阵
   * 在OpenGL显示时使用
   */
  @Suppress("unused")
  private fun getTextureMatrix(): FloatArray {
    mSurfaceTexture?.getTransformMatrix(mTextureMatrix)
    return mTextureMatrix
  }

  /**
   * 由c++回调回来
   * 创建硬编码
   * @param width 录制视频的宽
   * @param height 录制视频的高
   * @param videoBitRate 录制视频的码率
   * @param frameRate 录制视频的帧率
   */
  @Suppress("unused")
  private fun createMediaCodecSurfaceEncoderFromNative(width: Int, height: Int, videoBitRate: Int, frameRate: Int): Int {
    return try {
      val ret = mSurfaceEncoder.start(width, height, videoBitRate, frameRate)
      mSurface = mSurfaceEncoder.getInputSurface()
      ret
    } catch (e: Exception) {
      e.printStackTrace()
      -1
    }
  }

  /**
   * 由c++回调回来
   * 获取h264数据
   * @param data h264 buffer, 由c++创建传递回来
   * @return 返回h264数据的大小, 返回0时数据无效
   */
  @Suppress("unused")
  private fun drainEncoderFromNative(data: ByteArray): Int {
    return mSurfaceEncoder.drainEncoder(data)
  }

  /**
   * 由c++回调回来
   * 获取当前编码的时间, 毫秒
   * @return 当前编码的时间 mBufferInfo.presentationTimeUs
   */
  @Suppress("unused")
  private fun getLastPresentationTimeUsFromNative(): Long {
    return mSurfaceEncoder.getLastPresentationTimeUs()
  }

  /**
   * 由c++回调回来
   * 获取编码器的Surface
   * @return 编码器的Surface mediaCodec.createInputSurface()
   */
  @Suppress("unused")
  private fun getEncodeSurfaceFromNative(): Surface? {
    return mSurface
  }

  /**
   * 由c++回调回来
   * 发送end of stream信号给编码器
   */
  @Suppress("unused")
  private fun signalEndOfInputStream() {
    mSurfaceEncoder.signalEndOfInputStream()
  }

  /**
   * 由c++回调回来
   * 关闭mediaCodec编码器
   */
  @Suppress("unused")
  private fun closeMediaCodecCalledFromNative() {
    mSurfaceEncoder.release()
  }

  /**
   * 由c++回调回来
   * 释放当前的camera
   */
  @Suppress("unused")
  private fun releaseCameraFromNative() {
    Log.i(Constants.TAG, "enter releaseCameraFromNative")
    stopPreview()
    mSurfaceTexture?.setOnFrameAvailableListener(null)
    mSurfaceTexture = null
    Log.i(Constants.TAG, "leave releaseCameraFromNative")
  }

  /**
   * 创建c++对象
   * @return 返回c++对象地址, 返回 <=0 时创建失败
   */
  private external fun create(): Long

  /**
   * 初始化OpenGL上下文
   * @param handle c++对象指针地址
   * @param surface surface对象, 创建EGLSurface需要
   * @param width surface 宽, 创建EGLSurface需要
   * @param height surface 高, 创建EGLSurface需要
   * @param cameraFacingId 前后摄像头id
   */
  private external fun prepareEGLContext(handle: Long, surface: Surface, width: Int, height: Int)

  /**
   * 创建EGLSurface
   * @param handle c++对象指针地址
   * @param surface surface对象
   */
  private external fun createWindowSurface(handle: Long, surface: Surface)

  /**
   * 重新设置绘制区域大小
   * @param handle c++对象指针地址
   * @param width 需要绘制的宽
   * @param height 需要绘制的高
   */
  private external fun setRenderSize(handle: Long, width: Int, height: Int)

  /**
   * 绘制一帧内容
   * @param handle c++对象地址
   */
  private external fun onFrameAvailable(handle: Long)

  /**
   * 设置预览显示类型
   * @param handle Long
   * @param type Int 0: fix_xy 1: crop
   */
  private external fun setRenderType(handle: Long, type: Int)

  /**
   * 设置录制速度
   * @param handle c++地址
   * @param speed 录制的速度, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f
   */
  private external fun setSpeed(handle: Long, speed: Float)

  /**
   * 设置画幅到绘制和编码
   * @param handle c++对象地址
   * @param frame 画幅样式 0: 垂直 1: 横向 2: 方形
   */
  private external fun setFrame(handle: Long, frame: Int)

  /**
   * 设置texture矩阵
   * @param handle c++对象地址
   * @param matrix 矩阵
   */
  private external fun updateTextureMatrix(handle: Long, matrix: FloatArray)

  /**
   * 开始录制
   * @param handle c++对象地址
   * @param width 录制视频的宽
   * @param height 录制视频的高
   * @param videoBitRate 录制视频的码率
   * @param frameRate 录制视频的帧率
   * @param useHardWareEncode 是否使用硬编码
   * @param strategy
   */
  private external fun startEncode(handle: Long,
                                   path: String,
                                   width: Int,
                                   height: Int,
                                   videoBitRate: Int,
                                   frameRate: Int,
                                   useHardWareEncode: Boolean,
                                   audioSampleRate: Int,
                                   audioChannel: Int,
                                   audioBitRate: Int)

  /**
   * 停止录制
   * @param handle c++对象地址
   */
  private external fun stopEncode(handle: Long)

  /**
   * 销毁EGLSurface
   * @param handle c++对象地址
   */
  private external fun destroyWindowSurface(handle: Long)

  /**
   * 销毁OpenGL 上下文, 销毁之后, 不能在调用绘制函数, 不然会出错或者黑屏
   * @param handle c++对象地址
   */
  private external fun destroyEGLContext(handle: Long)

  private external fun switchCamera(handle: Long)

  /**
   * 添加一个滤镜
   * @param handle Long c++ 对象地址
   * @param configPath String 滤镜配置地址
   * @return Int 返回当前滤镜的唯一id
   */
  private external fun addFilter(handle: Long, configPath: String): Int

  /**
   * 更新一个滤镜
   * @param handle Long c++对象地址
   * @param configPath String 滤镜配置地址
   * @param startTime Int 滤镜开始时间
   * @param endTime Int 滤镜结束时间, 如果一直显示,可以传入Int.MAX
   * @param actionId Int 需要更新哪个滤镜, 必须为addFilter返回的actionId
   */
  private external fun updateFilter(handle: Long, configPath: String, startTime: Int, endTime: Int, actionId: Int)

  /**
   * 删除一个滤镜
   * @param handle Long c++对象地址
   * @param actionId Int 需要更新哪个滤镜, 必须为addFilter返回的actionId
   */
  private external fun deleteFilter(handle: Long, actionId: Int)

  /**
   * 删除c++对象
   * @param handle c++对象地址
   */
  private external fun release(handle: Long)
}