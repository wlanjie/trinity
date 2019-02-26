@file:Suppress("DEPRECATION")

package com.trinity.camera

import android.content.Context
import android.graphics.ImageFormat
import android.graphics.Rect
import android.graphics.SurfaceTexture
import android.hardware.Camera
import android.view.Surface
import android.view.WindowManager
import java.util.*

/**
 * Created by wlanjie on 2019/02/22.
 */
internal class Camera1(
    private val mContext: Context,
    private val mCameraCallback: CameraCallback) : TCamera {

  private var mCameraId: Int = -1
  private var mCameraInfo: Camera.CameraInfo = Camera.CameraInfo()
  private var mFacing: FacingId = FacingId.CAMERA_FACING_BACK
  private var mCamera: Camera ?= null
  private var mCameraParameters: Camera.Parameters ?= null
  private var mShowingPreview: Boolean = false
  private var mViewWidth = 0
  private var mViewHeight = 0

  private fun openCamera(surfaceTexture: SurfaceTexture) {
    if (mShowingPreview) {
      mCamera?.stopPreview()
    }
    mCamera?.release()
    mCamera = Camera.open(mCameraId)
    if (mCamera == null) {
      return
    }
    mCamera?.let {
      mCameraParameters = it.parameters
      val previewSizes = it.parameters.supportedPreviewSizes
      if (mCameraParameters?.isVideoStabilizationSupported == true) {
        mCameraParameters?.videoStabilization = true
      }
      val sizes = mutableListOf<Size>()
      previewSizes.forEach { size ->
        sizes.add(Size(size.width, size.height))
      }
      val previewSize = mCameraCallback.onPreviewSizeSelected(sizes)
      val fps = mCameraCallback.onPreviewFpsSelected(it.parameters.supportedPreviewFpsRange)
      mCameraParameters?.setPreviewFpsRange(fps[0], fps[1])
      mCameraParameters?.previewFormat = ImageFormat.NV21
      mCameraParameters?.setPreviewSize(previewSize.width, previewSize.height)
      mCamera?.setPreviewTexture(surfaceTexture)
      mCamera?.parameters = mCameraParameters
      mCamera?.setDisplayOrientation(computeSensorToViewOffset(getDisplayRotation()))

      val previewBuffer = ByteArray(previewSize.width * previewSize.height * 3 / 2)
      mCamera?.addCallbackBuffer(previewBuffer)
      mCamera?.setPreviewCallbackWithBuffer { data, _ ->
        if (data != null) {
          mCameraCallback.onPreviewCallback(data, previewSize.width, previewSize.height, PreviewFormat.NV21)
          mCamera?.addCallbackBuffer(previewBuffer)
        }
      }
      mCamera?.startPreview()
      mShowingPreview = true

      val modes = mCameraParameters?.supportedFlashModes
      mCameraCallback.onCameraFeatureSupported(it.parameters.isAutoExposureLockSupported,
              it.parameters.isAutoWhiteBalanceLockSupported,
              it.parameters.isSmoothZoomSupported,
              it.parameters.isVideoSnapshotSupported,
              it.parameters.isVideoStabilizationSupported,
              it.parameters.isZoomSupported,
              it.parameters.maxNumFocusAreas > 0,
              modes != null && modes.size > 0)
      mCameraCallback.onCameraOpened(previewSize.width, previewSize.height)
    }
  }

  private fun setAutoFocus() {
    mCameraParameters?.let {
      it.focusMode = mCameraCallback.onFocusModeSelected(it.supportedFocusModes)
    }
  }

  /**
   * start camera
   */
  override fun start(surfaceTexture: SurfaceTexture): Boolean {
    try {
      if (Camera.getNumberOfCameras() == 0) {
        return false
      }
      for (index in 0 until Camera.getNumberOfCameras()) {
        Camera.getCameraInfo(index, mCameraInfo)
        if (mCameraInfo.facing == mFacing.ordinal) {
          mCameraId = index
        }
      }
      if (mCameraId == -1) {
        return false
      }
      openCamera(surfaceTexture)
      return true
    } catch (e: Exception) {
      e.printStackTrace()
      return false
    }
  }

  /**
   * stop camera
   */
  override fun stop() {
    try {
      if (!mShowingPreview) {
        return
      }
      mCamera?.stopPreview()
      mShowingPreview = false
      mCamera?.release()
      mCamera = null
      mCameraCallback.onCameraClosed()
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  /**
   * switch camera
   */
  override fun setFacing(facing: FacingId) {
    mFacing = facing
  }

  /**
   * get camera id
   */
  override fun getFacing(): FacingId {
    return mFacing
  }

  /**
   * camera is opened
   */
  override fun isCameraOpened(): Boolean {
    return mCamera != null
  }

  override fun setViewSize(width: Int, height: Int) {
    mViewWidth = width
    mViewHeight = height
  }

  override fun setAutoFocus(enable: Boolean) {
    mCameraParameters?.let {
      try {
        if (enable) {
          it.focusMode = mCameraCallback.onFocusModeSelected(it.supportedFocusModes)
        } else {
          mCamera?.cancelAutoFocus()
          it.focusMode = Camera.Parameters.FOCUS_MODE_AUTO
        }
        mCamera?.parameters = it
      } catch (e: Exception) {
        e.printStackTrace()
      }
    }
  }

  /**
   * set camera zoom
   */
  override fun setZoom(zoom: Int) {
    try  {
      if (!(mCameraParameters != null && mCameraParameters?.isZoomSupported == true)) {
         throw IllegalStateException("zoom not supported")
      }
      if (zoom < 0 || zoom > getMaxZoom()) {
        throw IllegalStateException("zoom not support value = $zoom")
      }
      mCameraParameters?.zoom = zoom
      mCamera?.parameters = mCameraParameters
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  /**
   * set camera iso
   */
  override fun setIso(iso: Int) {
    try {
      if (!(mCameraParameters != null && mCameraParameters?.isAutoExposureLockSupported == true)) {
        throw IllegalStateException("auto exposure not supported")
      }
      val minExposureCompensation = mCameraParameters?.minExposureCompensation ?: -1
      val maxExposureCompensation = mCameraParameters?.maxExposureCompensation ?: -1
      if (iso < minExposureCompensation || iso > maxExposureCompensation) {
        throw IllegalStateException("iso not support value = $iso")
      }
      mCameraParameters?.exposureCompensation = iso
      mCamera?.parameters = mCameraParameters
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  private fun getDisplayRotation(): Int {
    val display = (mContext.getSystemService(Context.WINDOW_SERVICE) as WindowManager).defaultDisplay
    return when (display.rotation) {
      Surface.ROTATION_0 -> 0
      Surface.ROTATION_90 -> 90
      Surface.ROTATION_180 -> 180
      Surface.ROTATION_270 -> 270
      else -> 0
    }
  }

  /**
   * set focus Rect
   */
  override fun focus(x: Float, y: Float) {
    try {
      mCamera?.cancelAutoFocus()
      val meteringAreas2 = computeMeteringAreas(x.toDouble(), y.toDouble(), mViewWidth, mViewHeight, computeSensorToViewOffset(getDisplayRotation()))
      val meteringAreas1 = meteringAreas2.subList(0, 1)
      mCamera?.parameters?.let {
        val maxAF = it.maxNumFocusAreas
        val maxAE = it.maxNumMeteringAreas
        if (maxAF > 0) {
          it.focusAreas = if (maxAF > 1) meteringAreas2 else meteringAreas1
        }
        if (maxAE > 0) {
          it.meteringAreas = if (maxAE > 1) meteringAreas2 else meteringAreas1
        }
        it.focusMode = mCameraCallback.onFocusModeSelected(it.supportedFocusModes)
        mCamera?.parameters = it

        mCamera?.autoFocus { _, _ ->
          try {
            mCamera?.cancelAutoFocus()
            mCameraParameters?.focusMode = mCameraCallback.onFocusModeSelected(it.supportedFocusModes)
            mCamera?.parameters = mCameraParameters
          } catch (e: Exception) {
            e.printStackTrace()
          }
        }
      }
    } catch (e: Exception) {
      e.printStackTrace()
    }

  }

  private fun computeMeteringAreas(viewClickX: Double, viewClickY: Double, viewWidth: Int, viewHeight: Int, sensorToDisplay: Int): List<Camera.Area> {
    val displayToSensor = -sensorToDisplay
    val clickX = -1000.0 + viewClickX / viewWidth * 2000.0f
    val clickY = -1000.0 + viewClickY / viewHeight * 2000.0f

    // Apply rotation to this point.
    // https://academo.org/demos/rotation-about-point/
    val theta = displayToSensor.toDouble() * Math.PI / 180
    val sensorClickX = clickX * Math.cos(theta) - clickY * Math.sin(theta)
    val sensorClickY = clickX * Math.sin(theta) + clickY * Math.cos(theta)

    // Compute the rect bounds.
    val rect1 = computeMeteringArea(sensorClickX, sensorClickY, 150.0)
    val weight1 = 1000 // 150 * 150 * 1000 = more than 10.000.000
    val rect2 = computeMeteringArea(sensorClickX, sensorClickY, 300.0)
    val weight2 = 100 // 300 * 300 * 100 = 9.000.000

    val list = ArrayList<Camera.Area>(2)
    list.add(Camera.Area(rect1, weight1))
    list.add(Camera.Area(rect2, weight2))
    return list
  }

  private fun computeMeteringArea(centerX: Double, centerY: Double, size: Double): Rect {
    val delta = size / 2.0
    val top = Math.max(centerY - delta, -1000.0).toInt()
    val bottom = Math.min(centerY + delta, 1000.0).toInt()
    val left = Math.max(centerX - delta, -1000.0).toInt()
    val right = Math.min(centerX + delta, 1000.0).toInt()
    return Rect(left, top, right, bottom)
  }

  private fun computeSensorToViewOffset(displayOffset: Int): Int {
    Camera.getCameraInfo(mCameraId, mCameraInfo)
    return if (mCameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
      (360 - ((mCameraInfo.orientation + displayOffset) % 360)) % 360
    } else {
      (mCameraInfo.orientation - displayOffset + 360) % 360
    }
  }

  /**
   * get camera supported max zoom value
   */
  override fun getMaxZoom(): Float {
    return mCameraParameters?.maxZoom?.toFloat() ?: -1.0f
  }

  /**
   * get camera supported min iso value
   */
  override fun getMinIso(): Int {
    return mCameraParameters?.minExposureCompensation ?: -1
  }

  /**
   * get camera supported max iso value
   */
  override fun getMaxIso(): Int {
    return mCameraParameters?.maxExposureCompensation ?: -1
  }

  override fun getCurrentIso(): Int {
    return mCameraParameters?.exposureCompensation ?: -1
  }

  override fun setTorch(torch: Torch) {
    try {
      val modes = mCameraParameters?.supportedFlashModes
      if (modes == null || modes.isEmpty()) {
        return
      }
      if (modes.contains(Camera.Parameters.FLASH_MODE_OFF) && torch == Torch.OFF) {
        mCameraParameters?.flashMode = Camera.Parameters.FLASH_MODE_OFF
      } else {
        mCameraParameters?.flashMode = Camera.Parameters.FLASH_MODE_TORCH
      }
      mCamera?.parameters = mCameraParameters
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  override fun getCameraDisplayOrientation(): Int {
      val degrees = getDisplayRotation()
      val info = Camera.CameraInfo()
      Camera.getCameraInfo(mCameraId, info)
      return if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
        (info.orientation + degrees) % 360
      } else {
        (info.orientation - degrees + 360) % 360
      }
  }
}