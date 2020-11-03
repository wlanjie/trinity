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
import android.media.AudioRecord
import android.provider.Settings
import android.provider.Settings.SettingNotFoundException
import android.view.OrientationEventListener
import android.view.Surface
import com.tencent.mars.xlog.Log
import com.trinity.BuildConfig
import com.trinity.Constants
import com.trinity.ErrorCode
import com.trinity.OnRecordingListener
import com.trinity.camera.*
import com.trinity.core.Frame
import com.trinity.core.MusicInfo
import com.trinity.core.RenderType
import com.trinity.editor.VideoExportInfo
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
import java.text.SimpleDateFormat
import java.util.*

/**
 * Create by wlanjie on 2019/3/14-下午8:45
 */
class TrinityRecord(
  val context: Context,
  preview: TrinityPreviewView
) : TrinityPreviewView.PreviewViewCallback, Timer.OnTimerListener, CameraCallback {

  // c++Object pointer address
  private var mHandle: Long = 0
  // Camera object
  private var mCamera = Camera1(preview.context, this)
  // Preview width
  private var mCameraWidth = 0
  // Preview high
  private var mCameraHeight = 0
  private var mViewWidth = 0
  private var mViewHeight = 0
  private var mFrame = Frame.CROP
  private var mCameraCallback: CameraCallback ?= null
  // Rendering type
  private var mRenderType = RenderType.CROP
  // Request to open the width and height of the camera
  private var mResolution = PreviewResolution.RESOLUTION_1280x720
  // Preview scale
  private var mAspectRatio = AspectRatio.of(16, 9)
  // surface texture
  private var mSurfaceTexture: SurfaceTexture ?= null
  // texture matrix
  private val mTextureMatrix = FloatArray(16)
  // Hardcoded object
  private var mSurfaceEncoder = MediaCodecSurfaceEncoder()
  // Surface Object
  private var mSurface: Surface ?= null
  // Whether to create for the first time
  // Create OpenGL environment when first created, and start OpenGL thread
  // Create EGLSurface directly the second time
  private var mFirst = true
  // Surface does it exist
  // Set to true in createSurface
  // Set to false in destroySurface
  private var mSurfaceExist = false
  // Whether to stop
  private var mStop = false
  // Audio recording
  private val mAudioRecordService: RecorderService = AudioRecordRecorderServiceImpl.instance
  // music player
  private var mPlayerService: PlayerService ?= null
  // Timer
  private var mTimer = Timer(this, 20)
  // Is the music playing
  // When this flag is true, when recording starts, continue playing
  private var mMusicPlaying = false
  // texture callback
  // Can do special effects textureId is of OES type
  private var mOnRenderListener: OnRenderListener ?= null
  // Recording event
  private var mOnRecordingListener: OnRecordingListener ?= null
  // Is recording
  private var mRecording = false
  // music player
  private var mAudioPlayer: AudioPlayer ?= null
  // Music information object
  private var mMusicInfo: MusicInfo ?= null
  // Recording speed
  private var mSpeed = Speed.NORMAL
  // Whether to request to open the camera
  // Set to true if textureId is not created yet
  // When the textureId is created successfully,Turn on the camera
  private var mRequestPreview = false
  // Face detection example
  private var mFaceDetection: FaceDetection ?= null
  // Equipment angle detection
  private val mOrientationListener: OrientationEventListener
  // Equipment angle
  private var mRotateDegree = 0
  // Whether the device automatically rotates
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

  // SurfaceView surfaceChanged Callback
  override fun resetRenderSize(width: Int, height: Int) {
    mViewWidth = width
    mViewHeight = height
    // Resize the drawing area
    setRenderSize(mHandle, width, height)
  }

  // SurfaceView surfaceDestroyed Callback
  override fun destroySurface() {
    Log.i(Constants.TAG, "destroySurface")
    destroyEGLContext()
    mSurfaceExist = false
  }

  private fun startPreview(surface: Surface, width: Int, height: Int) {
    if (mFirst) {
      // Create a c++ object for the first time
      mHandle = create()
      // Initialize the OpenGL context and start the thread
      prepareEGLContext(mHandle, surface, width, height)
      setRenderType(mRenderType)
      setFrame(mFrame)
      Log.d("tag", "startPreview")
      mFirst = false
    } else {
      // Create EGLSurface directly
      createWindowSurface(mHandle, surface)
    }
    mSurfaceExist = true
  }

  private fun destroyEGLContext() {
    // Release the OpenGL context, Stop thread
    destroyEGLContext(mHandle)
    // Delete c++ object
    release(mHandle)
    mHandle = 0
    mFirst = true
    mSurfaceExist = false
    mStop = false
  }

  /**
   * Set preview display type
   * @param renderType FIX_XY Show the whole screen,At the same time crop the display, CROP is displayed in original scale,Stay black up and down at the same time
   */
  fun setRenderType(renderType: RenderType) {
    mRenderType = renderType
    setRenderType(mHandle, renderType.ordinal)
  }

  /**
   * Set playback speed,Invalid setting during recording
   * note: When recording quickly,Need to deal with dropped frames,Otherwise, the recorded video can reach 120fps at 4 times faster speed
   * @param speed Speed
   */
  fun setSpeed(speed: Speed) {
    mSpeed = speed
  }

  /**
   * Set frame
   * The current frame has: Vertical horizontal 1:1 square
   */
  fun setFrame(frame: Frame) {
    mFrame = frame
    setFrame(mHandle, frame.ordinal)
  }

  /**
   * Set recording callback
   */
  fun setOnRecordingListener(l: OnRecordingListener) {
    mOnRecordingListener = l
  }

  /**
   * Set render callback
   */
  fun setOnRenderListener(l: OnRenderListener) {
    mOnRenderListener = l
  }

  /**
   * Set background music path
   * @return 0 success
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
   * Turn on camera preview
   * @param resolution PreviewResolution Preview resolution selection
   */
  fun startPreview(resolution: PreviewResolution) {
    mResolution = resolution
    if (mSurfaceTexture == null) {
      // The opengl environment is not ready yet,Cannot turn on the camera at this time
      // Because the camera needs the textureId of OES to open
      mRequestPreview = true
      return
    }
    mSurfaceTexture?.let {
      mCamera.setAspectRatio(mAspectRatio)
      mCamera.start(it, mResolution)
    }
  }

  /**
   * Stop preview, Release camera
   */
  fun stopPreview() {
    mCamera.stop()
    mRequestPreview = false
  }

  /**
   * Switch camera
   */
  fun switchCamera() {
    mCamera.setAspectRatio(mAspectRatio)
    val facing = if (getCameraFacing() == Facing.BACK) Facing.FRONT else Facing.BACK
    mCamera.setFacing(facing)
    switchCamera(mHandle)
  }

  /**
   * Set the current camera id
   */
  @Suppress("unused")
  fun setCameraFacing(facing: Facing) {
    mCamera.setFacing(facing)
  }

  /**
   * Get the current camera id
   * @return Facing Returns the enumeration definition of the front and rear cameras
   */
  @Suppress("unused")
  fun getCameraFacing(): Facing {
    return mCamera.getFacing()
  }

  /**
   * Flash at the beginning
   * @param flash
   * Flash.OFF shut down
   * Flash.ON Turn on
   * Flash.TORCH
   * Flash.AUTO automatic
   */
  fun flash(flash: Flash) {
    mCamera.setFlash(flash)
  }

  /**
   * Set camera zoom
   * 100 Maximum zoom
   * @param zoom camera zoom value, Max 100, Minimum 0
   */
  fun setZoom(zoom: Int) {
    mCamera.setZoom(zoom)
  }

  /**
   * Set camera exposure
   * 100 Is the largest
   * @param exposureCompensation camera exposure value, Last 100, Minimum 0
   */
  @Suppress("unused")
  fun setExposureCompensation(exposureCompensation: Int) {
    mCamera.setExposureCompensation(exposureCompensation)
  }

  /**
   * Set focus area
   * @param point X of the focus area, y value
   */
  fun focus(point: PointF) {
    mCamera.focus(mViewWidth, mViewHeight, point)
  }

  /**
   * Set up face detection interface
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
   * End of recording
   */
  override fun end(timer: Timer) {
    stopRecording()
  }

  @Suppress("unused")
  private fun isLandscape(displayOrientation: Int): Boolean {
    return false
  }


  /**
   * Add filter
   * Such as: content.The absolute path of json is /sdcard/Android/com.trinity.sample/cache/filters/config.json
   * The incoming path only needs /sdcard/Android/com.trinity.sample/cache/filters Can
   * If the current path does not contain config.json Add failed
   * The specific json content is as follows:
   * {
   *  "type": 0,
   *  "intensity": 1.0,
   *  "lut": "lut_124/lut_124.png"
   * }
   *
   * json Parameter explanation:
   * type: reserved text, Currently no effect
   * intensity: Filter transparency, 0.0 No difference between time and camera capture
   * lut: The address of the filter color table, Must be a local address, And relative path
   *      sdk Internal path stitching
   * @param configPath Filter config.json Parent directory
   * @return Returns the current filter一id
   */
  @Suppress("unused")
  fun addFilter(configPath: String): Int {
    return addFilter(mHandle, configPath)
  }

  /**
   * Update filter
   * @param configPath config.jsonroute of, Currently symmetric addFilter Description
   * @param startTime Start time of the filter
   * @param endTime The end time of the filter
   * @param actionId Which filter needs to be updated, Must be the actionId returned by addFilter
   */
  @Suppress("unused")
  fun updateFilter(configPath: String, startTime: Int, endTime: Int, actionId: Int) {
    updateFilter(mHandle, configPath, startTime, endTime, actionId)
  }

  /**
   * Update filter transparency
   * @param intensity Float Filter transparency 0.0 ~ 1.0
   * @param actionId Int addAction Id returned when
   */
  fun updateFilterIntensity(intensity: Float, actionId: Int) {
    updateFilterIntensity(mHandle, intensity, actionId)
  }

  private external fun updateFilterIntensity(handle: Long, intensity: Float, actionId: Int)

  /**
   * Remove filter
   * @param actionId Which filter needs to be removed, Must be the actionId returned when addFilter
   */
  fun deleteFilter(actionId: Int) {
    deleteFilter(mHandle, actionId)
  }

  /**
   * Add special effects
   * Such as: content.json The absolute path is /sdcard/Android/com.trinity.sample/cache/effects/config.json
   * The incoming path only needs /sdcard/Android/com.trinity.sample/cache/effects Can
   * If the current path does not contain config.json Add failed
   * @param configPath Filter config.json Parent directory
   * @return Returns the unique id of the current effect
   */
  fun addEffect(configPath: String): Int {
    if (mHandle <= 0) {
      return -1
    }
    return addEffect(mHandle, configPath)
  }

  private external fun addEffect(handle: Long, config: String): Int

  /**
   * Update specific effects
   * @param startTime The start time of the special effect
   * @param endTime The end time of the special effect
   * @param actionId Which special effect needs to be updated, Must be the actionId returned by addAction
   */
  fun updateEffectTime(startTime: Int, endTime: Int, actionId: Int) {
    if (mHandle <= 0) {
      return
    }
    updateEffectTime(mHandle, startTime, endTime, actionId)
  }

  private external fun updateEffectTime(handle: Long, startTime: Int, endTime: Int, actionId: Int)

  /**
   * Update the parameters of the specified special effect
   * @param actionId Int Which special effect needs to be updated, Must be the actionId returned by addAction
   * @param effectName String Need to update the name of the effect, This is set in json
   * @param paramName String Update OpenGL Parameter values in shader
   * @param value Float Specific parameter value 0.0 ~ 1.0
   */
  fun updateEffectParam(actionId: Int, effectName: String, paramName: String, value: Float) {
    updateEffectParam(mHandle, actionId, effectName, paramName, value)
  }

  private external fun updateEffectParam(handle: Long, actionId: Int, effectName: String, paramName: String, value: Float)

//  /**
//   * Update the parameters of the specified special effect
//   * @param actionId Int Which special effect needs to be updated, Must be the actionId returned by addAction
//   * @param effectName String Need to update the name of the effect, This is set in json
//   * @param paramName String Update OpenGL Parameter values in shader
//   * @param value FloatArray Specific parameter value 0.0 ~ 1.0
//   */
//  fun updateActionParam(actionId: Int, effectName: String, paramName: String, value: FloatArray) {
//    updateActionParam(mHandle, actionId, effectName, paramName, value)
//  }
//
//  private external fun updateActionParam(handle: Long, actionId: Int, effectName: String, paramName: String, value: FloatArray)

  /**
   * Delete a special effect
   * @param actionId Which special effect needs to be deleted, Must be the actionId returned by addAction
   */
  fun deleteEffect(actionId: Int) {
    if (mHandle <= 0) {
      return
    }
    deleteEffect(mHandle, actionId)
  }

  private external fun deleteEffect(handle: Long, actionId: Int)


  /**
   * Set recording angle
   * @param rotation Angle contains 0 90 180 270
   */
  @Suppress("unused")
  fun setRecordRotation(rotation: Int) {

  }

  /**
   * Set mute recording
   * For silent recording, you need to set all the data collected by the microphone to 0.
   * @param mute true is mute
   */
  @Suppress("unused")
  fun setMute(mute: Boolean) {
//    if (mute) {
//      mAudioRecordService.stop()
//    }else{
//
//    }
  }

  private fun getNow(): String {
    return if (android.os.Build.VERSION.SDK_INT >= 24){
      SimpleDateFormat("yyyy-MM-dd-HH-mm", Locale.US).format(Date())
    } else {
      val tms = Calendar.getInstance()
      tms.get(Calendar.YEAR).toString() + "-" +
              tms.get(Calendar.MONTH).toString() + "-" +
              tms.get(Calendar.DAY_OF_MONTH).toString() + "-" +
              tms.get(Calendar.HOUR_OF_DAY).toString() + "-" +
              tms.get(Calendar.MINUTE).toString()
    }
  }

  /**
   * Start recording a video
   * @param path The address where the recorded video is saved
   * @param width The width of the recorded video, The SDK will do 16 times integer operations, The width of the final output video may be inconsistent with the setting
   * @param height High of recorded video, The SDK will do 16 times integer operations, The width of the final output video may be inconsistent with the setting
   * @param videoBitRate Video output bit rate, If it is set to 2000, Then 2M, The final output and the setting may be different
   * @param frameRate Video output frame rate
   * @param useHardWareEncode Whether to use hard coding, If set to true, And hard coding does not support,Soft coding
   * @param audioSampleRate Audio sampling rate
   * @param audioChannel Number of audio channels
   * @param audioBitRate Audio bit rate
   * @param duration How much time needs to be recorded
   * @return Int ErrorCode.SUCCESS For success,Other failures
   * @throws InitRecorderFailException
   */
  @Throws(InitRecorderFailException::class)
  fun startRecording(info: VideoExportInfo, duration: Int): Int {
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
      val tag = "trinity-" + BuildConfig.VERSION_NAME + "-" + getNow()
      startEncode(
        mHandle, info.path, info.width, info.height, info.videoBitRate, info.frameRate,
        info.mediaCodecEncode,
        info.sampleRate, info.channelCount, info.audioBitRate,
        tag)
      mRecording = true
    }
    return ErrorCode.SUCCESS
  }

  /**
   * Stop recording
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
   * In activity Called in the onDestroy method,Release other resources
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
   * Get face information, Called by c++
   * @return Array<FaceDetectionReport>? Face information collection
   */
  @Suppress("unused")
  private fun getFaceDetectionReports() = mFaceDetectionReports

  /**
   * Get preview width，Called by c++，Create framebuffer
   */
  @Suppress("unused")
  private fun getCameraWidth(): Int {
    if (mSurfaceTexture == null) {
      return 0
    }
    return mCamera.getWidth()
  }

  /**
   * Get preview width，Called by c++，Create framebuffer
   */
  @Suppress("unused")
  private fun getCameraHeight(): Int {
    if (mSurfaceTexture == null) {
      return 0
    }
    return mCamera.getHeight()
  }

  /**
   * Call back by c++
   * Open preview
   * @param textureId OES type id
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
   * Call back by c++
   * Refresh the content captured by the camera
   */
  @Suppress("unused")
  private fun updateTexImageFromNative() {
    mSurfaceTexture?.updateTexImage()
  }

  /**
   * surface creation callback
   * Callback by c++
   */
  @Suppress("unused")
  private fun onSurfaceCreated() {
    mOnRenderListener?.onSurfaceCreated()
  }

  /**
   * texture callback
   * Can do special effects, textureId is TEXTURE_2D type
   * @param textureId Camera textureId
   * @param width texture width
   * @param height high texture
   * @return Return the processed textureId, Must be TEXTURE_2D type
   */
  @Suppress("unused")
  private fun onDrawFrame(textureId: Int, width: Int, height: Int): Int {
    return mOnRenderListener?.onDrawFrame(textureId, width, height, mTextureMatrix) ?: -1
  }

  /**
   * surface destruction callback
   *Callback by c++
   */
  @Suppress("unused")
  private fun onSurfaceDestroy() {
    mOnRenderListener?.onSurfaceDestroy()
  }

  /**
   * Called by c++
   * Get texture matrix
   * Used in OpenGL display
   */
  @Suppress("unused")
  private fun getTextureMatrix(): FloatArray {
    mSurfaceTexture?.getTransformMatrix(mTextureMatrix)
    return mTextureMatrix
  }

  /**
   * Call back by c++
   * Create hard code
   * @param width The width of the recorded video
   * @param height High of recorded video
   * @param videoBitRate Bit rate of recorded video
   * @param frameRate Frame rate of recorded video
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
   * Call back by c++
   * Get h264 data
   * @param data h264 buffer, Created by c++ and passed back
   * @return Returns the size of h264 data, Data is invalid when 0 is returned
   */
  @Suppress("unused")
  private fun drainEncoderFromNative(data: ByteArray): Int {
    return mSurfaceEncoder.drainEncoder(data)
  }

  /**
   *Call back by c++
   * Get the current encoding time, millisecond
   * @return Current encoding time mBufferInfo.presentationTimeUs
   */
  @Suppress("unused")
  private fun getLastPresentationTimeUsFromNative(): Long {
    return mSurfaceEncoder.getLastPresentationTimeUs()
  }

  /**
   * Call back by c++
   * Get the Surface of the encoder
   * @return Surface of the encoder mediaCodec.createInputSurface()
   */
  @Suppress("unused")
  private fun getEncodeSurfaceFromNative(): Surface? {
    return mSurface
  }

  /**
   * Call back by c++
   * Send the end of stream signal to the encoder
   */
  @Suppress("unused")
  private fun signalEndOfInputStream() {
    mSurfaceEncoder.signalEndOfInputStream()
  }

  /**
   * Call back by c++
   * Close mediaCodec encoder
   */
  @Suppress("unused")
  private fun closeMediaCodecCalledFromNative() {
    mSurfaceEncoder.release()
  }

  /**
   * Call back by c++
   * Release the current camera
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
   * Create c++ objects
   * @return Return the address of the c++ object, Creation failed when return <=0
   */
  private external fun create(): Long

  /**
   * Initialize OpenGL context
   * @param handle c++ Object pointer address
   * @param surface surface object, Need to create EGLSurface
   * @param width surface width, Need to create EGLSurface
   * @param height surface high, Need to create EGLSurface
   * @param cameraFacingId Front and rear camera id
   */
  private external fun prepareEGLContext(handle: Long, surface: Surface, width: Int, height: Int)

  /**
   * Create EGLSurface
   * @param handle c++ Object pointer address
   * @param surface surface object
   */
  private external fun createWindowSurface(handle: Long, surface: Surface)

  /**
   * Reset the drawing area size
   * @param handle c++ Object pointer address
   * @param width The width to be drawn
   * @param height Need to draw high
   */
  private external fun setRenderSize(handle: Long, width: Int, height: Int)

  /**
   * Draw a frame of content
   * @param handle c++ Object address
   */
  private external fun onFrameAvailable(handle: Long)

  /**
   * Set preview display type
   * @param handle Long
   * @param type Int 0: fix_xy 1: crop
   */
  private external fun setRenderType(handle: Long, type: Int)

  /**
   * Set recording speed
   * @param handle c++ address
   * @param speed Recording speed, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f
   */
  private external fun setSpeed(handle: Long, speed: Float)

  /**
   * Set frame to draw and encode
   * @param handle c++ Object address
   * @param frame Frame style 0: vertical 1: Horizontal 2: Square
   */
  private external fun setFrame(handle: Long, frame: Int)

  /**
   * Set the texture matrix
   * @param handle c++ Object address
   * @param matrix matrix
   */
  private external fun updateTextureMatrix(handle: Long, matrix: FloatArray)

  /**
   * Start recording
   * @param handle c++ Object address
   * @param width The width of the recorded video
   * @param height High of recorded video
   * @param videoBitRate Bit rate of recorded video
   * @param frameRate Bit rate of recorded video
   * @param useHardWareEncode Whether to use hard coding
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
                                   audioBitRate: Int,
                                   tag: String)

  /**
   * Stop recording
   * @param handle c++ Object address
   */
  private external fun stopEncode(handle: Long)

  /**
   * Destroy EGLSurface
   * @param handle c++ Object address
   */
  private external fun destroyWindowSurface(handle: Long)

  /**
   * Destroy OpenGL Context, After destruction, Cannot call drawing function, Otherwise there will be an error or a black screen
   * @param handle c++ Object address
   */
  private external fun destroyEGLContext(handle: Long)

  private external fun switchCamera(handle: Long)

  /**
   * Add a filter
   * @param handle Long c++ Object address
   * @param configPath String Filter configuration address
   * @return Int Returns the unique id of the current filter
   */
  private external fun addFilter(handle: Long, configPath: String): Int

  /**
   * Update a filter
   * @param handle Long c++ Object address
   * @param configPath String Filter configuration address
   * @param startTime Int Filter start time
   * @param endTime Int Filter end time, If it keeps showing,Int can be passed in.MAX
   * @param actionId Int Which filter needs to be updated, Must be the actionId returned by addFilter
   */
  private external fun updateFilter(handle: Long, configPath: String, startTime: Int, endTime: Int, actionId: Int)

  /**
   * Remove a filter
   * @param handle Long c++ Object address
   * @param actionId Int Which filter needs to be updated, Must be the actionId returned by addFilter
   */
  private external fun deleteFilter(handle: Long, actionId: Int)

  /**
   * Delete c++ object
   * @param handle c++ object address
   */
  private external fun release(handle: Long)
}