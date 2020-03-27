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

@file:Suppress("DEPRECATION")

package com.trinity.camera

import android.content.Context
import android.graphics.ImageFormat
import android.graphics.PointF
import android.graphics.Rect
import android.graphics.SurfaceTexture
import android.os.Handler
import com.tencent.mars.xlog.Log
import com.trinity.record.PreviewResolution
import java.lang.Exception
import kotlin.math.max
import kotlin.math.min

/**
 * Created by wlanjie on 2019-07-15
 */

class Camera1(
  private val mContext: Context,
  private val mCameraCallback: CameraCallback
) : Camera, android.hardware.Camera.ErrorCallback {

  private val mHandler = Handler()
  private val mAngles = Angles()
  private var mCameraId = -1
  private var mCamera: android.hardware.Camera ?= null
  private var mCameraParameters: android.hardware.Camera.Parameters ?= null
  private val mCameraInfo = android.hardware.Camera.CameraInfo()
  private val mPreviewSize = SizeMap()
  private var mAspectRatio: AspectRatio ?= AspectRatio.of(16, 9)
  private var mShowingPreview = false
  private var mAutoFocus = true
  private var mFacing = Facing.BACK
  private var mFlash = Flash.AUTO
  private var mWhiteBalance = WhiteBalance.AUTO
  private var mDisplayOrientation = 0
  private var mPreviewStreamSize: Size ?= null
  private var mSurfaceTexture: SurfaceTexture ?= null
  private var mFocusEndRunnable: Runnable ?= null
  private var mResolution = PreviewResolution.RESOLUTION_1280x720
  private var mZoom = 0
  private var mExposureCompensation = 0

  companion object {
    private const val AUTOFOCUS_END_DELAY_MILLIS: Long = 2500
    private val FLASH_MODES = mutableMapOf<Flash, String>()
    private val WHITE_BALANCE_MODES = mutableMapOf<WhiteBalance, String>()

    init {
      FLASH_MODES[Flash.OFF] = android.hardware.Camera.Parameters.FLASH_MODE_OFF
      FLASH_MODES[Flash.ON] = android.hardware.Camera.Parameters.FLASH_MODE_ON
      FLASH_MODES[Flash.TORCH] = android.hardware.Camera.Parameters.FLASH_MODE_TORCH
      FLASH_MODES[Flash.AUTO] = android.hardware.Camera.Parameters.FLASH_MODE_AUTO
      FLASH_MODES[Flash.RED_EYE] = android.hardware.Camera.Parameters.FLASH_MODE_RED_EYE

      WHITE_BALANCE_MODES[WhiteBalance.AUTO] = android.hardware.Camera.Parameters.WHITE_BALANCE_AUTO
      WHITE_BALANCE_MODES[WhiteBalance.CLOUDY] = android.hardware.Camera.Parameters.WHITE_BALANCE_CLOUDY_DAYLIGHT
      WHITE_BALANCE_MODES[WhiteBalance.DAYLIGHT] = android.hardware.Camera.Parameters.WHITE_BALANCE_DAYLIGHT
      WHITE_BALANCE_MODES[WhiteBalance.FLUORESCENT] = android.hardware.Camera.Parameters.WHITE_BALANCE_FLUORESCENT
      WHITE_BALANCE_MODES[WhiteBalance.INCANDESCENT] = android.hardware.Camera.Parameters.WHITE_BALANCE_INCANDESCENT
    }
  }

  private fun chooseCamera() {
    for (index in 0 until android.hardware.Camera.getNumberOfCameras()) {
      android.hardware.Camera.getCameraInfo(index, mCameraInfo)
      if (mCameraInfo.facing == mFacing.ordinal) {
        setSensorOffset(mFacing, mCameraInfo.orientation)
        mCameraId = index
        break
      }
    }
  }

  private fun chooseAspectRatio(): AspectRatio? {
    mPreviewSize.ratios().forEach {
      if (it == mAspectRatio) {
        return it
      }
    }
    return null
  }

  private fun setAutoFocusInternal() {

  }

  private fun isLandscape(orientationDegrees: Int): Boolean {
    return orientationDegrees == 90 || orientationDegrees == 270
  }

  private fun calcCameraRotation(screenOrientationDegrees: Int): Int {
    return if (mCameraInfo.facing == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
      (mCameraInfo.orientation + screenOrientationDegrees) % 360
    } else {
      val landscapeFlip = if (isLandscape(screenOrientationDegrees)) 180 else 0
      (mCameraInfo.orientation + screenOrientationDegrees + landscapeFlip) % 360
    }
  }

  private fun applyFocusMode(params: android.hardware.Camera.Parameters) {
    val modes = params.supportedFocusModes
    if (modes.contains(android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
      params.focusMode = android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO
      return
    }
    if (modes.contains(android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
      params.focusMode = android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE
      return
    }
    if (modes.contains(android.hardware.Camera.Parameters.FOCUS_MODE_INFINITY)) {
      params.focusMode = android.hardware.Camera.Parameters.FOCUS_MODE_INFINITY
      return
    }
    if (modes.contains(android.hardware.Camera.Parameters.FOCUS_MODE_FIXED)) {
      params.focusMode = android.hardware.Camera.Parameters.FOCUS_MODE_FIXED
      return
    }
  }

  private fun applyFlash(params: android.hardware.Camera.Parameters) {
    val modes = params.supportedFlashModes ?: return
    val mode = FLASH_MODES[mFlash]
    mode?.let {
      if (modes.contains(it)) {
        params.flashMode = it
      }
    }
  }

  private fun applyWhiteBalance(params: android.hardware.Camera.Parameters) {
    val modes = params.supportedWhiteBalance ?: return
    val mode = WHITE_BALANCE_MODES[mWhiteBalance]
    mode?.let {
      if (modes.contains(it)) {
        params.whiteBalance = it
      }
    }
  }

  private fun applyZoom(params: android.hardware.Camera.Parameters, zoom: Int) {
    if (params.isZoomSupported) {
      val max = params.maxZoom
      params.zoom = (zoom / 100f * max).toInt()
    }
  }

  private fun applyExposureCorrection(params: android.hardware.Camera.Parameters, exposureCorrection: Int) {
    if (params.isAutoExposureLockSupported) {
      val max = params.maxExposureCompensation
      val min = params.minExposureCompensation
      val value = if (exposureCorrection <= 50) {
        exposureCorrection / 50f * min
      } else {
        (exposureCorrection - 50f) / 50f * max
      }
      params.exposureCompensation = value.toInt()
    }
  }

  // TODO
  private fun getPreviewSurfaceSize(resolution: PreviewResolution): Size {
    return Size(resolution.width, resolution.height)
  }

  private fun computePreviewStreamSize(resolution: PreviewResolution): Size {
    val previewSizes = mPreviewSize.sizes(mAspectRatio)
    val flip = mAngles.flip(Reference.SENSOR, Reference.VIEW)
    val sizes = ArrayList<Size>(previewSizes.size)
    previewSizes.forEach {
      sizes.add(if (flip) it.flip() else it)
    }
    val targetMinSize = getPreviewSurfaceSize(resolution)
    if (flip) {
      mAspectRatio = mAspectRatio?.flip()
    }
    val matchRatio = SizeSelectors.and(
      SizeSelectors.aspectRatio(mAspectRatio, 0f),
      SizeSelectors.biggest()
    )
    val matchSize = SizeSelectors.and(
      SizeSelectors.minHeight(targetMinSize.height),
      SizeSelectors.minWidth(targetMinSize.width),
      SizeSelectors.smallest()
    )
    val matchAll = SizeSelectors.or(
      SizeSelectors.and(matchRatio, matchSize),
      matchSize,
      matchRatio,
      SizeSelectors.biggest()
    )
    val matchSizes = matchAll.select(sizes)
    matchSizes.forEach {
      var result = it
      if (flip) {
        result = it.flip()
      }
      if (result.width == resolution.width && result.height == resolution.height) {
        return result
      }
    }
    var result = matchAll.select(sizes)[0]
    if (!sizes.contains(result)) {
      throw RuntimeException("SizeSelectors must not return Sizes other than those in the input list.")
    }
    if (flip) {
      result = result.flip()
    }
    return result
  }

  private fun openCamera(resolution: PreviewResolution): Int {
    mCamera?.release()
    if (mCameraId == -1) {
      return -1
    }
    Log.i("trinity", "request preview width: ${resolution.width} height: ${resolution.height}")
    try {
      mCamera = android.hardware.Camera.open(mCameraId)
    } catch (e: RuntimeException) {
      e.printStackTrace()
      return -1
    }
    mCamera?.setErrorCallback(this)
    val params = mCamera?.parameters
    mPreviewSize.clear()
    params?.supportedPreviewSizes?.forEach {
      mPreviewSize.add(Size(it.width, it.height))
    }
    params?.previewFormat = ImageFormat.NV21
    params?.let {
      applyFocusMode(it)
      applyFlash(it)
      applyWhiteBalance(it)
      applyZoom(it, mZoom)
      applyExposureCorrection(it, mExposureCompensation)
    }
    val size = computePreviewStreamSize(resolution)
    Log.i("trinity", "setPreviewSize width: ${size.width} height: ${size.height}")
    params?.setPreviewSize(size.width, size.height)
    mCamera?.parameters = params
    mCameraParameters = params
    mCamera?.setDisplayOrientation(mAngles.offset(Reference.SENSOR, Reference.VIEW, Axis.ABSOLUTE))
    val cameraInfo = android.hardware.Camera.CameraInfo()
    android.hardware.Camera.getCameraInfo(mCameraId, cameraInfo)
    mCamera?.setPreviewCallback { data, _ ->
      mCameraCallback.dispatchOnPreviewCallback(data, size.width, size.height, cameraInfo.orientation)
    }
    return 0
  }

  private fun setSensorOffset(facing: Facing, sensorOffset: Int) {
    mAngles.setSensorOffset(facing, sensorOffset)
  }

  private fun setDisplayOffset(displayOffset: Int) {
    mAngles.setDisplayOffset(displayOffset)
  }

  private fun setDeviceOrientation(deviceOrientation: Int) {
    mAngles.setDeviceOrientation(deviceOrientation)
  }

  override fun start(surfaceTexture: SurfaceTexture, resolution: PreviewResolution): Boolean {
    mResolution = resolution
    mSurfaceTexture = surfaceTexture
    chooseCamera()
    openCamera(resolution)
    mCamera?.setPreviewTexture(surfaceTexture)
    mShowingPreview = true
    mCamera?.startPreview()
    return true
  }

  override fun stop() {
    mCamera?.setPreviewCallback(null)
    mCamera?.stopPreview()
    mShowingPreview = false
    mCamera?.release()
    mCamera = null
  }

  override fun isCameraOpened(): Boolean {
    return mCamera != null
  }

  override fun setFacing(facing: Facing) {
    if (mFacing == facing) {
      return
    }
    mFacing = facing
    if (isCameraOpened()) {
      stop()
      mSurfaceTexture?.let {
        start(it, mResolution)
      }
    }
  }

  override fun getFacing(): Facing {
    return mFacing
  }

  override fun getSupportAspectRatios(): Set<AspectRatio> {
    val idealAspectRatios = mPreviewSize
//    idealAspectRatios.ratios().forEach {
//    }
    return idealAspectRatios.ratios()
  }

  override fun setAspectRatio(ratio: AspectRatio): Boolean {
    mAspectRatio = ratio
    return false
  }

  override fun setZoom(zoom: Int) {
    if (mCameraParameters == null) {
      mZoom = zoom
    } else {
      mCameraParameters?.let {
        applyZoom(it, zoom)
        mCamera?.parameters = it
      }
    }
  }

  override fun setExposureCompensation(exposureCompensation: Int) {
    if (mCameraParameters == null) {
      mExposureCompensation = exposureCompensation
    } else {
      mCameraParameters?.let {
        applyExposureCorrection(it, exposureCompensation)
        mCamera?.parameters = it
      }
    }
  }

  override fun setFlash(flash: Flash) {
    if (flash.ordinal == mFlash.ordinal) {
      return
    }
    mCameraParameters?.let {
      mFlash = flash
      applyFlash(it)
      mCamera?.parameters = it
    }
  }

  override fun getFlash(): Flash {
    return mFlash
  }

  override fun setDisplayOrientation(displayOrientation: Int) {
    mDisplayOrientation = displayOrientation
  }

  override fun getWidth(): Int {
    return mCamera?.parameters?.previewSize?.width ?: 0
  }

  override fun getHeight(): Int {
    return mCamera?.parameters?.previewSize?.height ?: 0
  }

  override fun onError(error: Int, camera: android.hardware.Camera?) {
  }

  override fun focus(viewWidth: Int, viewHeight: Int, point: PointF) {
    if (mFacing == Facing.FRONT) {
      return
    }
    mHandler.post {
      val p = PointF(point.x, point.y) // copy.
      val offset = mAngles.offset(Reference.SENSOR, Reference.VIEW, Axis.ABSOLUTE)
      val meteringAreas2 = computeMeteringAreas(p.x.toDouble(), p.y.toDouble(),
          viewWidth, viewHeight, offset)
      val meteringAreas1 = meteringAreas2.subList(0, 1)

      mCamera?.parameters?.let {
        val maxAF = it.maxNumFocusAreas
        val maxAE = it.maxNumMeteringAreas
        if (maxAF > 0) it.focusAreas = if (maxAF > 1) meteringAreas2 else meteringAreas1
        if (maxAE > 0) it.meteringAreas = if (maxAE > 1) meteringAreas2 else meteringAreas1
        it.focusMode = android.hardware.Camera.Parameters.FOCUS_MODE_AUTO
        try {
          mCamera?.parameters = it
        } catch (e: Exception) {
          e.printStackTrace()
        }
      }
      mCameraCallback.dispatchOnFocusStart(p)

      mFocusEndRunnable?.let {
        mHandler.removeCallbacks(it)
      }
      mFocusEndRunnable = Runnable { mCameraCallback.dispatchOnFocusEnd(false, p) }
      mFocusEndRunnable?.let {
        mHandler.postDelayed(it, AUTOFOCUS_END_DELAY_MILLIS)
      }

      try {
        mCamera?.autoFocus { success, _ ->
          mFocusEndRunnable?.let {
            mHandler.removeCallbacks(it)
          }
          mFocusEndRunnable = null
          mCameraCallback.dispatchOnFocusEnd(success, p)
          mHandler.removeCallbacks(mFocusResetRunnable)
          mHandler.postDelayed(mFocusResetRunnable, 3000)
        }
      } catch (e: RuntimeException) {
      }
    }
  }

  private fun computeMeteringAreas(clickX: Double, clickY: Double,
                                   viewWidth: Int, viewHeight: Int,
                                   sensorToDisplay: Int): List<android.hardware.Camera.Area> {
    var viewClickX = clickX
    var viewClickY = clickY
    // Event came in view coordinates. We must rotate to sensor coordinates.
    // First, rescale to the -1000 ... 1000 range.
    val displayToSensor = -sensorToDisplay
    viewClickX = -1000.0 + viewClickX / viewWidth.toDouble() * 2000.0
    viewClickY = -1000.0 + viewClickY / viewHeight.toDouble() * 2000.0

    // Apply rotation to this point.
    // https://academo.org/demos/rotation-about-point/
    val theta = displayToSensor.toDouble() * Math.PI / 180
    val sensorClickX = viewClickX * Math.cos(theta) - viewClickY * Math.sin(theta)
    val sensorClickY = viewClickX * Math.sin(theta) + viewClickY * Math.cos(theta)

    // Compute the rect bounds.
    val rect1 = computeMeteringArea(sensorClickX, sensorClickY, 150.0)
    val weight1 = 1000 // 150 * 150 * 1000 = more than 10.000.000
    val rect2 = computeMeteringArea(sensorClickX, sensorClickY, 300.0)
    val weight2 = 100 // 300 * 300 * 100 = 9.000.000

    val list = java.util.ArrayList<android.hardware.Camera.Area>(2)
    list.add(android.hardware.Camera.Area(rect1, weight1))
    list.add(android.hardware.Camera.Area(rect2, weight2))
    return list
  }

  private fun computeMeteringArea(centerX: Double, centerY: Double, size: Double): Rect {
    val delta = size / 2.0
    val top = max(centerY - delta, -1000.0).toInt()
    val bottom = min(centerY + delta, 1000.0).toInt()
    val left = max(centerX - delta, -1000.0).toInt()
    val right = min(centerX + delta, 1000.0).toInt()
    return Rect(left, top, right, bottom)
  }

  private val mFocusResetRunnable = Runnable {
    mCamera?.cancelAutoFocus()
    val params = mCamera?.parameters
    val maxAF = params?.maxNumFocusAreas ?: 0
    val maxAE = params?.maxNumMeteringAreas ?: 0
    if (maxAF > 0) params?.focusAreas = null
    if (maxAE > 0) params?.meteringAreas = null
    params?.let {
      applyFocusMode(it)
      mCamera?.parameters = it
    }
  }

}