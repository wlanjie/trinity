package com.trinity.camera

import android.content.Context
import android.graphics.SurfaceTexture

/**
 * Created by wlanjie on 2019-07-15
 */

class Camera1(
  private val mContext: Context,
  private val mCameraCallback: CameraCallback
) : Camera {

  private var mCameraId = -1
  private var mCamera: android.hardware.Camera ?= null
  private var mCameraParameters: android.hardware.Camera.Parameters ?= null
  private val mCameraInfo = android.hardware.Camera.CameraInfo()
  private val mPreviewSize = SizeMap()
  private var mAspectRatio: AspectRatio ?= AspectRatio.of(16, 9)
  private var mShowingPreview = false
  private var mAutoFocus = true
  private var mFacing = 0
  private var mFlash = Flash.AUTO
  private var mDisplayOrientation = 0

  companion object {
    private val FLASH_MODES = mutableMapOf<Flash, String>()

    init {
      FLASH_MODES[Flash.OFF] = android.hardware.Camera.Parameters.FLASH_MODE_OFF
      FLASH_MODES[Flash.ON] = android.hardware.Camera.Parameters.FLASH_MODE_ON
      FLASH_MODES[Flash.TORCH] = android.hardware.Camera.Parameters.FLASH_MODE_TORCH
      FLASH_MODES[Flash.AUTO] = android.hardware.Camera.Parameters.FLASH_MODE_AUTO
      FLASH_MODES[Flash.RED_EYE] = android.hardware.Camera.Parameters.FLASH_MODE_RED_EYE
    }
  }

  private fun chooseCamera() {
    for (index in 0 until android.hardware.Camera.getNumberOfCameras()) {
      android.hardware.Camera.getCameraInfo(index, mCameraInfo)
      if (mCameraInfo.facing == mFacing) {
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

  private fun openCamera(): Int {
    mCamera?.release()
    if (mCameraId == -1) {
      return -1
    }
    mCamera = android.hardware.Camera.open(mCameraId)
    mCameraParameters = mCamera?.parameters
    mPreviewSize.clear()
    mCameraParameters?.supportedPreviewSizes?.forEach {
      mPreviewSize.add(Size(it.width, it.height))
    }
    adjustCameraParameters()
    mCamera?.setDisplayOrientation(mDisplayOrientation)
    return 0
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
    if (mFacing == facing) {
      return
    }
    mFacing = facing
    if (isCameraOpened()) {
      stop()
//      start()
    }
  }

  override fun getFacing(): Int {
    return mFacing
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
}