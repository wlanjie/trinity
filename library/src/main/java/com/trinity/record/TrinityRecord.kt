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

import android.graphics.PointF
import android.graphics.SurfaceTexture
import android.hardware.Camera
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
  preview: TrinityPreviewView
) : TrinityPreviewView.PreviewViewCallback, Timer.OnTimerListener, CameraCallback {

  // c++对象指针地址
  private var mHandle: Long = 0
  // 前后摄像头id
  private var mCameraFacingId = Camera.CameraInfo.CAMERA_FACING_FRONT
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
  private var mSurfaceEncoder: MediaCodecSurfaceEncoder ?= null
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

  init {
    preview.setCallback(this)
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
    mAudioPlayer = AudioPlayer()
    // TODO seek
//    mAudioPlayer?.seek()
    mMusicInfo = info
    return ErrorCode.SUCCESS
  }

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
   * 获取当前摄像头id
   */
  fun getCameraFacing(): Facing {
    return mCamera.getFacing()
  }

  /**
   * 设置开启和关闭闪光灯
   */
  fun flash(flash: Flash) {
    mCamera.setFlash(flash)
  }

  /**
   * 设置camera缩放
   * 100 为最大缩放
   */
  fun setZoom(zoom: Int) {
    mCamera.setZoom(zoom)
  }

  /**
   * 设置camera曝光度
   * 100 为最大
   */
  fun setExposureCompensation(exposureCompensation: Int) {
    mCamera.setExposureCompensation(exposureCompensation)
  }

  /**
   * 设置聚焦区域
   */
  fun focus(point: PointF) {
    mCamera.focus(mViewWidth, mViewHeight, point)
  }

  override fun dispatchOnFocusStart(where: PointF) {
    mCameraCallback?.dispatchOnFocusStart(where)
  }

  override fun dispatchOnFocusEnd(success: Boolean, where: PointF) {
    mCameraCallback?.dispatchOnFocusEnd(success, where)
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

  private fun isLandscape(displayOrientation: Int): Boolean {
    return false
  }

  /**
   * 添加一个特效, 包含普通滤镜等
   * @param config json 内容, 底层根据解析json来判断添加什么效果
   * @return Int 返回当前特效的id, 在更新和删除一个特效时必须传入
   */
  fun addAction(config: String): Int {
    return 0
  }

  /**
   * 更新一个特效
   * @param config 更新的json内容,底层根据解析json来判断更新什么效果
   * @param actionId Int 创建action时返回的id
   */
  fun updateAction(config: String, actionId: Int) {

  }

  /**
   * 删除一个特效
   * @param actionId 创建action时返回的id
   */
  fun deleteAction(actionId: Int) {

  }

  /**
   * 设置录制角度
   * @param rotation 角度包含 0 90 180 270
   */
  fun setRecordRotation(rotation: Int) {

  }

  /**
   * 设置静音录制
   * 静音录制需要把麦克风采集到的数据全部设置为0即可
   * @param mute true为静音
   */
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
        mAudioRecordService.init()
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
      mAudioPlayer?.release()
      mTimer.release()
      mPlayerService?.stopAccompany()
      mPlayerService?.stop()
    }
  }

  /**
   * 获取预览宽，由c++调用，创建framebuffer
   */
  @Suppress("unused")
  private fun getCameraWidth() = mCamera.getWidth()

  /**
   * 获取预览宽，由c++调用，创建framebuffer
   */
  @Suppress("unused")
  private fun getCameraHeight() = mCamera.getHeight()

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
      it.setOnFrameAvailableListener {
        onFrameAvailable(mHandle)
      }
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
   * 可以做特效处理, textureId为OES类型
   * @param textureId 相机textureId OES类型
   * @param width texture宽
   * @param height texture高
   * @return 返回处理过的textureId, 必须为TEXTURE_2D类型
   */
  @Suppress("unused")
  private fun onDrawFrame(textureId: Int, width: Int, height: Int): Int {
    return mOnRenderListener?.onDrawFrame(textureId, width, height, mTextureMatrix) ?: 0
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
  private fun createMediaCodecSurfaceEncoderFromNative(width: Int, height: Int, videoBitRate: Int, frameRate: Int) {
    try {
      mSurfaceEncoder = MediaCodecSurfaceEncoder(width, height, videoBitRate, frameRate)
      mSurface = mSurfaceEncoder?.inputSurface
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  /**
   * 由c++回调回来
   * 调整编码信息
   * @param width 重新调整编码视频的宽
   * @param height 重新调整编码视频的高
   * @param videoBitRate 重新调整编码视频的码率
   * @param fps 重新调整编码视频的帧率
   */
  @Suppress("unused")
  private fun hotConfigEncoderFromNative(width: Int, height: Int, videoBitRate: Int, fps: Int) {
    mSurfaceEncoder?.hotConfig(width, height, videoBitRate, fps)
    mSurface = mSurfaceEncoder?.inputSurface
  }

  /**
   * 由c++回调回来
   * 获取h264数据
   * @param data h264 buffer, 由c++创建传递回来
   * @return 返回h264数据的大小, 返回0时数据无效
   */
  @Suppress("unused")
  private fun pullH264StreamFromDrainEncoderFromNative(data: ByteArray): Long {
    return mSurfaceEncoder?.pullH264StreamFromDrainEncoderFromNative(data) ?: 0
  }

  /**
   * 由c++回调回来
   * 获取当前编码的时间, 毫秒
   * @return 当前编码的时间 mBufferInfo.presentationTimeUs
   */
  @Suppress("unused")
  private fun getLastPresentationTimeUsFromNative(): Long {
    return mSurfaceEncoder?.lastPresentationTimeUs ?: 0
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
   * 关闭mediaCodec编码器
   */
  @Suppress("unused")
  private fun closeMediaCodecCalledFromNative() {
    mSurfaceEncoder?.shutdown()
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
   * 删除c++对象
   * @param handle c++对象地址
   */
  private external fun release(handle: Long)
}