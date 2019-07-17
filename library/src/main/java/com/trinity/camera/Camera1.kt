package com.trinity.camera

import android.content.Context
import android.graphics.SurfaceTexture

/**
 * Created by wlanjie on 2019-07-15
 */

class Camera1(
  private val mContext: Context,
  private val mCameraCallback: CameraCallback
) : Camera, android.hardware.Camera.ErrorCallback {
  override fun onError(error: Int, camera: android.hardware.Camera?) {
    TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
  }

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

  companion object {
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

  private fun setFlashInternal(flash: Flash): Boolean {
    if (isCameraOpened()) {
      val modes = mCameraParameters?.supportedFlashModes
      val mode = FLASH_MODES[flash]
      if (mode != null && modes?.contains(mode) == true) {
        mCameraParameters?.flashMode = mode
        mFlash = flash
        return true
      }
      val currentMode = FLASH_MODES[flash]
      if (modes?.contains(currentMode) == false) {
        mCameraParameters?.flashMode = android.hardware.Camera.Parameters.FLASH_MODE_OFF
        mFlash = Flash.OFF
      }
    } else {
      mFlash = flash
    }
    return false
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

  private fun adjustCameraParameters() {
    var sizes = mPreviewSize.sizes(mAspectRatio)
    if (sizes == null) {
      mAspectRatio = chooseAspectRatio()
      sizes = mPreviewSize.sizes(mAspectRatio)
    }
    var size = mCameraCallback.onPreviewSizeSelected(sizes) ?: throw IllegalArgumentException("size is null")
    mCameraParameters?.setPreviewSize(size.width, size.height)
    mCameraParameters?.setRotation(calcCameraRotation(mDisplayOrientation))
    setAutoFocusInternal()
    setFlashInternal(mFlash)
    mCamera?.parameters = mCameraParameters
    if (mShowingPreview) {
      mCamera?.startPreview()
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
    val modes = params.supportedFlashModes
    val mode = FLASH_MODES[mFlash]
    if (modes.contains(mode)) {
      params.flashMode = mode
    }
  }

  private fun applyWhiteBalance(params: android.hardware.Camera.Parameters) {
    val modes = params.supportedWhiteBalance
    val mode = WHITE_BALANCE_MODES[mWhiteBalance]
    if (modes.contains(mode)) {
      params.whiteBalance = mode
    }
  }

  private fun applyZoom(params: android.hardware.Camera.Parameters, zoom: Float) {
    if (params.isZoomSupported) {
      val max = params.maxZoom
      params.zoom = (zoom * max).toInt()
    }
  }

  private fun applyExposureCorrection(params: android.hardware.Camera.Parameters, exposureCorrection: Int) {
    if (params.isAutoExposureLockSupported) {
      val max = params.maxExposureCompensation
      val min = params.minExposureCompensation
      val value = if (exposureCorrection < min) {
        min
      } else {
        if (exposureCorrection > max) {
          max
        } else {
          exposureCorrection
        }
      }
      params.exposureCompensation = value.toInt()
    }
  }

  private fun getPreviewSurfaceSize(reference: Reference): Size {
    return Size(720, 1280)
  }

  private fun computePreviewStreamSize(): Size {
    val previewSizes = mPreviewSize.sizes(mAspectRatio)
    val flip = mAngles.flip(Reference.SENSOR, Reference.VIEW)
    val sizes = ArrayList<Size>(previewSizes.size)
    previewSizes.forEach {
      sizes.add(if (flip) it.flip() else it)
    }
    val targetMinSize = getPreviewSurfaceSize(Reference.VIEW)
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
    val selector = matchAll
    var result = selector.select(sizes)[0]
    if (!sizes.contains(result)) {
      throw RuntimeException("SizeSelectors must not return Sizes other than those in the input list.")
    }
    if (flip) {
      result = result.flip()
    }
    return result
  }

  private fun openCamera(): Int {
    mCamera?.release()
    if (mCameraId == -1) {
      return -1
    }
    mCamera = android.hardware.Camera.open(mCameraId)
    mCamera?.setErrorCallback(this)
    val params = mCamera?.parameters
    mPreviewSize.clear()
    params?.supportedPreviewSizes?.forEach {
      mPreviewSize.add(Size(it.width, it.height))
    }
    params?.let {
      applyFocusMode(it)
      applyFlash(it)
      applyWhiteBalance(it)
      applyZoom(it, 0f)
      applyExposureCorrection(it, 0)
    }
    val size = computePreviewStreamSize()
    params?.setPreviewSize(size.width, size.height)
    mCamera?.parameters = params
    mCameraParameters = params
    mCamera?.setDisplayOrientation(mAngles.offset(Reference.SENSOR, Reference.VIEW, Axis.ABSOLUTE))
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

  override fun start(surfaceTexture: SurfaceTexture): Boolean {
    chooseCamera()
    openCamera()
    mCamera?.setPreviewTexture(surfaceTexture)
    mShowingPreview = true
    mCamera?.startPreview()
    return true
  }

  override fun stop() {
    mCamera?.stopPreview()
    mShowingPreview = false
    mCamera?.release()
    mCamera = null
  }

  override fun isCameraOpened(): Boolean {
    return mCamera != null
  }

  override fun setFacing(facing: Int) {
//    if (mFacing == facing) {
//      return
//    }
//    mFacing = facing
//    if (isCameraOpened()) {
//      stop()
////      start()
//    }
  }

  override fun getFacing(): Int {
    return mFacing.ordinal
  }

  override fun getSupportAspectRatios(): Set<AspectRatio> {
    val idealAspectRatios = mPreviewSize
//    idealAspectRatios.ratios().forEach {
//    }
    return idealAspectRatios.ratios()
  }

  override fun setAspectRatio(ratio: AspectRatio): Boolean {
    if (mAspectRatio == null || !isCameraOpened()) {
      mAspectRatio = ratio
      return true
    } else if (mAspectRatio?.equals(ratio) == false) {
      val sizes = mPreviewSize.sizes(ratio)
      if (sizes == null) {
        throw UnsupportedOperationException("$ratio is not supported")
      } else {
        mAspectRatio = ratio
        adjustCameraParameters()
        return true
      }
    }
    return false
  }

  override fun setFlash(flash: Flash) {
    if (flash.ordinal == mFlash.ordinal) {
      return
    }
    if (setFlashInternal(flash)) {
      mCamera?.parameters = mCameraParameters
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
}