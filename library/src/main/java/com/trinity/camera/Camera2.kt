package com.innotech.camera

import android.annotation.SuppressLint
import android.annotation.TargetApi
import android.content.Context
import android.graphics.*
import android.hardware.camera2.*
import android.hardware.camera2.params.MeteringRectangle
import android.media.Image
import android.media.ImageReader
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import android.util.Range
import android.util.Rational
import android.view.Surface
import android.view.WindowManager
import com.trinity.camera.*
import java.util.*

/**
 * Created by wlanjie on 2018/1/18.
 *
 * 参考 Camera2Kit https://github.com/lytcom/Camera2Kit
 * Camera2App https://android.googlesource.com/platform/packages/apps/Camera2/
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
internal class Camera2(context: Context, private val mCameraCallback: CameraCallback, private val mSurfaceTexture: SurfaceTexture) :
  SVCamera {

  private val YUV420P = 0
  private val YUV420SP = 1
  private val NV21 = 2
  private val GCAM_METERING_REGION_FRACTION = 0.1225f
  private val mWindowManager: WindowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
  private val mCameraManager: CameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
  private var mCamera: CameraDevice ?= null
  private var mCaptureSession: CameraCaptureSession ?= null
  private var mCameraCharacteristics: CameraCharacteristics ?= null
  private var mPreviewRequestBuilder: CaptureRequest.Builder ?= null
  private var mBackgroundThread: HandlerThread ?= null
  private var mBackgroundHandler: Handler ?= null
  private var mFacing: FacingId = FacingId.CAMERA_FACING_BACK
  private var mCameraId: String = ""
  private var mCurrentZoom = 1
  private var mViewWidth = 0
  private var mViewHeight = 0
  private var mImageReader: ImageReader ?= null

  private fun chooseCameraIdByFacing(): Boolean {
    try {
      val cameraIds = mCameraManager.cameraIdList
      if (cameraIds.isEmpty()) {
        return false
      }
      for (id in cameraIds) {
        val characteristics = mCameraManager.getCameraCharacteristics(id)
        val level = characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)
        if (level == null || level == CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
          continue
        }
        val internal = characteristics.get(CameraCharacteristics.LENS_FACING) ?: continue
        if (mFacing == FacingId.CAMERA_FACING_FRONT) {
          if (CameraMetadata.LENS_FACING_FRONT == internal) {
            mCameraId = id
            mCameraCharacteristics = characteristics
            return true
          }
        } else if (mFacing == FacingId.CAMERA_FACING_BACK) {
          if (CameraMetadata.LENS_FACING_BACK == internal) {
            mCameraId = id
            mCameraCharacteristics = characteristics
            return true
          }
        }
      }
      // not found
      mCameraId = cameraIds[0]
      mCameraCharacteristics = mCameraManager.getCameraCharacteristics(mCameraId)
      if (mCameraCharacteristics == null) {
        return false
      }
      val level = mCameraCharacteristics?.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)
      if (level == null || level == CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
        return false
      }
      val internal = mCameraCharacteristics?.get(CameraCharacteristics.LENS_FACING) ?: return false

      mFacing = if (internal == CameraMetadata.LENS_FACING_FRONT) FacingId.CAMERA_FACING_FRONT else FacingId.CAMERA_FACING_BACK
      return true
    } catch (e: Exception) {
      e.printStackTrace()
      return false
    }
  }

  private fun getBytesFromImageReader(image: Image, yuv: ByteArray, u: ByteArray, v: ByteArray, type: Int, bytes: ByteArray) {
    val planes = image.planes
    val width = image.width
    val height = image.height
    var dstIndex = 0
    var uIndex = 0
    var vIndex = 0

    var pixelStride: Int
    var rowStride: Int

    planes.forEachIndexed { index, plane ->
      pixelStride = plane.pixelStride
      rowStride = plane.rowStride
      val buffer = plane.buffer

//      val bytes = ByteArray(buffer.capacity())
      buffer.get(bytes, 0, buffer.capacity())
      var srcIndex = 0

      when (index) {
        0 -> for (j in 0 until height) {
          System.arraycopy(bytes, srcIndex, yuv, dstIndex, width)
          srcIndex += rowStride
          dstIndex += width
        }
        1 -> for (j in 0 until height / 2) {
          for (k in 0 until width / 2) {
            u[uIndex++] = bytes[srcIndex]
            srcIndex += pixelStride
          }
          if (pixelStride == 2) {
            srcIndex += rowStride - width
          } else if (pixelStride == 1) {
            srcIndex += rowStride - width / 2
          }
        }
        else -> for (j in 0 until height / 2) {
          for (k in 0 until width / 2) {
            v[vIndex++] = bytes[srcIndex]
            srcIndex += pixelStride
          }
          if (pixelStride == 2) {
            srcIndex += rowStride - width
          } else if (pixelStride == 1) {
            srcIndex += rowStride - width / 2
          }
        }
      }
    }

    when (type) {
      YUV420P -> {
        System.arraycopy(u, 0, yuv, dstIndex, u.size)
        System.arraycopy(v, 0, yuv, dstIndex + u.size, v.size)
      }
      YUV420SP -> for (i in 0 until v.size) {
        yuv[dstIndex++] = u[i]
        yuv[dstIndex++] = v[i]
      }
      NV21 -> for (i in 0 until v.size) {
        yuv[dstIndex++] = v[i]
        yuv[dstIndex++] = u[i]
      }
    }
  }

  private fun createImageReader(width: Int, height: Int) {
    mImageReader = ImageReader.newInstance(width, height, ImageFormat.YUV_420_888, 2)
    val yuv = ByteArray(width * height * ImageFormat.getBitsPerPixel(ImageFormat.YUV_420_888) / 8)
    val u = ByteArray(width * height / 4)
    val v = ByteArray(width * height / 4)
    val bytes = ByteArray(yuv.size)
    mImageReader?.setOnImageAvailableListener({
      try {
        val image = it.acquireNextImage()
        getBytesFromImageReader(image, yuv, u, v, NV21, bytes)
        image.close()
      } catch (e: IllegalStateException) {
        e.printStackTrace()
      }
    }, null)
  }

  private fun startPreview(width: Int, height: Int) {
    if (!isCameraOpened()) {
      return
    }

//    val frameRateBackOff = Range(7, 25)
//    mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, frameRateBackOff)

    createImageReader(width, height)
    mSurfaceTexture.setDefaultBufferSize(width, height)
    try {
      mPreviewRequestBuilder = mCamera?.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW) ?: return
      if (mPreviewRequestBuilder == null) {
        return
      }
      val surface = Surface(mSurfaceTexture)
      val imageReaderSurface = mImageReader?.surface
      val surfaces = Arrays.asList(surface, imageReaderSurface)
      mPreviewRequestBuilder?.addTarget(surface)
      mPreviewRequestBuilder?.addTarget(imageReaderSurface)

//      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//        mCamera?.createConstrainedHighSpeedCaptureSession(surfaces, object : CameraCaptureSession.StateCallback() {
//          override fun onConfigureFailed(session: CameraCaptureSession?) {
//          }
//
//          override fun onConfigured(session: CameraCaptureSession?) {
//          }
//
//        }, mBackgroundHandler)
//      }

      mCamera?.createCaptureSession(surfaces, object: CameraCaptureSession.StateCallback() {
        override fun onConfigureFailed(session: CameraCaptureSession?) {

        }

        override fun onConfigured(session: CameraCaptureSession?) {
          if (session == null) {
            return
          }

          try {
            mCaptureSession = session
            updateAutoFocus()
            if (isFlashSupported()) {
              mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH)
            }
            // Auto focus should be continuous for camera preview
            session.setRepeatingRequest(mPreviewRequestBuilder?.build(), object : CameraCaptureSession.CaptureCallback() {

              override fun onCaptureCompleted(session: CameraCaptureSession?, request: CaptureRequest?, result: TotalCaptureResult?) {
                super.onCaptureCompleted(session, request, result)
              }

              override fun onCaptureFailed(session: CameraCaptureSession?, request: CaptureRequest?, failure: CaptureFailure?) {
                super.onCaptureFailed(session, request, failure)
                Log.e("SureVideo", "onCaptureFailed")
              }
            }, mBackgroundHandler)
          } catch (e: Exception) {
            e.printStackTrace()
          }
        }

      }, mBackgroundHandler)
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  private fun chooseOptimalSize(): Size? {
    if (mCameraCharacteristics == null) {
      return null
    }
    val map = mCameraCharacteristics?.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP) ?: return null

    val outputSizes = map.getOutputSizes(SurfaceTexture::class.java)
    val size = mutableListOf<Size>()
    outputSizes.forEach {
      size.add(Size(it.width, it.height))
    }
    return mCameraCallback.onPreviewSizeSelected(size)
  }

  override fun configCamera(facing: FacingId): CameraConfigInfo {
    TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
  }

  @SuppressLint("MissingPermission")
  override fun start(surfaceTexture: SurfaceTexture): Boolean {
    try {
      if (!chooseCameraIdByFacing()) {
        return false
      }
      if (mCameraManager == null || mCameraManager.cameraIdList.isEmpty()) {
        return false
      }
      mBackgroundThread = HandlerThread("CameraBackground").also { it.start() }
      mBackgroundHandler = Handler(mBackgroundThread?.looper)

      mCameraManager.openCamera(mCameraId, object: CameraDevice.StateCallback() {
        override fun onOpened(camera: CameraDevice?) {
          val size = chooseOptimalSize() ?: return
          mCamera = camera
          startPreview(size.width, size.height)
        }

        override fun onDisconnected(camera: CameraDevice?) {
          mCamera?.close()
          mCamera = null
        }

        override fun onError(camera: CameraDevice?, error: Int) {
          Log.e("SureVideo", "open camera: $mCameraId error: $error")
          mCamera = null
        }

      }, mBackgroundHandler)
      return true
    } catch (e: Exception) {
      e.printStackTrace()
      return false
    }
  }

  override fun stop() {
    mCaptureSession?.close()
    mCaptureSession = null
    mCamera?.close()
    mCamera = null
    mImageReader?.close()
    mBackgroundThread?.looper?.quit()
    mBackgroundThread?.quitSafely()
    try {
      mBackgroundThread?.join()
      mBackgroundThread = null
      mBackgroundHandler = null
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  override fun setFacing(facing: FacingId) {
    mFacing = facing
  }

  override fun getFacing(): FacingId {
    return mFacing
  }

  override fun setViewSize(width: Int, height: Int) {
    mViewWidth = width
    mViewHeight = height
  }

  override fun setAutoFocus(enable: Boolean) {
    if (enable) {
      updateAutoFocus()
    } else {
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
    }
  }

  override fun isCameraOpened(): Boolean {
    return mCamera != null
  }

  private fun getZoomCropRegion(zoom: Int): Rect {
    val sensor = mCameraCharacteristics?.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE) ?: return Rect()
    val xCenter = sensor.width() / 2
    val yCenter = sensor.height() / 2
    val xDelta = (0.5f * sensor.width() / zoom).toInt()
    val yDelta = (0.5f * sensor.height() / zoom).toInt()
    return Rect(xCenter - xDelta, yCenter - yDelta, xCenter + xDelta, yCenter + yDelta)
  }

  override fun setZoom(zoom: Int) {
    // camera2 PreviewOverlay 126行 setupZoom
    // maxZoom 4.0
    mCurrentZoom = zoom
    val cropRegion = getZoomCropRegion(zoom)
    mPreviewRequestBuilder?.set(CaptureRequest.SCALER_CROP_REGION, cropRegion)
    mCaptureSession?.setRepeatingBurst(Arrays.asList(mPreviewRequestBuilder?.build()), object: CameraCaptureSession.CaptureCallback() {

    }, mBackgroundHandler)
  }

  override fun setIso(iso: Int) {
    if (mCamera == null) {
      return
    }
    mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, iso)
    mCaptureSession?.setRepeatingBurst(Arrays.asList(mPreviewRequestBuilder?.build()), object: CameraCaptureSession.CaptureCallback() {

    }, mBackgroundHandler)
  }

  private fun transformPortraitCoordinatesToSensorCoordinates(point: PointF, sensorOrientation: Int): PointF {
    return when (sensorOrientation) {
      0 -> point
      90 -> PointF(point.y, 1.0f - point.x)
      180 -> PointF(1.0f - point.x, 1.0f - point.y)
      270 -> PointF(1.0f - point.y, point.x)
      else ->
        // Impossible exception.
        throw IllegalArgumentException("Unsupported Sensor Orientation")
    }
  }

  private fun clamp(value: Int, min: Int, max: Int): Int {
    return Math.min(Math.max(value, min), max)
  }

  /**
   * @Return The weight to use for [MeteringRectangle]s for 3A.
   */
  private fun getMeteringWeight(): Int {
    // TODO Determine the optimal metering region for non-HDR photos.
    val weightMin = MeteringRectangle.METERING_WEIGHT_MIN
    val weightRange = MeteringRectangle.METERING_WEIGHT_MAX - MeteringRectangle.METERING_WEIGHT_MIN
    return (weightMin + GCAM_METERING_REGION_FRACTION * weightRange).toInt()
  }

  private fun getAERegions(point: PointF, cropRegion: Rect): Array<MeteringRectangle> {
    val minCropEdge = Math.min(cropRegion.width(), cropRegion.height())
    val halfSideLength = (0.5f * GCAM_METERING_REGION_FRACTION * minCropEdge).toInt()
    val sensorOrientation = mCameraCharacteristics?.get(CameraCharacteristics.SENSOR_ORIENTATION) ?: 0
    val nsc = transformPortraitCoordinatesToSensorCoordinates(point, sensorOrientation)

    val xCenterSensor = (cropRegion.left + nsc.x * cropRegion.width()).toInt()
    val yCenterSensor = (cropRegion.top + nsc.y * cropRegion.height()).toInt()

    val meteringRegion = Rect(xCenterSensor - halfSideLength,
        yCenterSensor - halfSideLength,
        xCenterSensor + halfSideLength,
        yCenterSensor + halfSideLength)

    // Clamp meteringRegion to cropRegion.
    meteringRegion.left = clamp(meteringRegion.left, cropRegion.left, cropRegion.right)
    meteringRegion.top = clamp(meteringRegion.top, cropRegion.top, cropRegion.bottom)
    meteringRegion.right = clamp(meteringRegion.right, cropRegion.left, cropRegion.right)
    meteringRegion.bottom = clamp(meteringRegion.bottom, cropRegion.top, cropRegion.bottom)
    return Array(1, {MeteringRectangle(meteringRegion, getMeteringWeight())})
  }

  override fun focus(x: Float, y: Float) {
    try {
      if (mViewWidth == 0 || mViewHeight == 0) {
        return
      }

      val cropRegion = getZoomCropRegion(mCurrentZoom)
      // RectF 0, 0, width, height
      val previewArea = RectF(0f, 0f, mViewWidth.toFloat(), mViewHeight.toFloat())
      val points = FloatArray(2)
      points[0] = (x - previewArea.left) / previewArea.width()
      points[1] = (y - previewArea.top) / previewArea.height()

      val rotationMatrix = Matrix()
      // displayRotation ?
      val displayRotation = mWindowManager.defaultDisplay.rotation
      rotationMatrix.setRotate(displayRotation.toFloat(), 0.5f, 0.5f)
      rotationMatrix.mapPoints(points)

      if (getFacing() === FacingId.CAMERA_FACING_FRONT) {
        points[0] = 1 - points[0]
      }

      val point = PointF(points[0], points[1])
      val meteringRectangles = getAERegions(point, cropRegion)

      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_REGIONS, meteringRectangles)
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_REGIONS, meteringRectangles)
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)

      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_AUTO)
      if (isAutoExposureSupported()) {
        mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH)
      }
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_START)
      mCaptureSession?.capture(mPreviewRequestBuilder?.build(), object : CameraCaptureSession.CaptureCallback() {

      }, mBackgroundHandler)
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_IDLE)
      mCaptureSession?.setRepeatingBurst(Arrays.asList(mPreviewRequestBuilder?.build()), object : CameraCaptureSession.CaptureCallback() {

      }, mBackgroundHandler)
      mBackgroundHandler?.removeCallbacks(mAutoFocusRunnable)
      mBackgroundHandler?.postDelayed(mAutoFocusRunnable, 3000)
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  private val mAutoFocusRunnable = Runnable {
    try {
      if (mPreviewRequestBuilder == null) {
        return@Runnable
      }
      updateAutoFocus()
      mCaptureSession?.setRepeatingRequest(mPreviewRequestBuilder?.build(), null, mBackgroundHandler)
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  private fun updateAutoFocus() {
    if (!isAutoFocusSupported()) {
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
    } else {
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
    }
    val zeroMeteringRectangle = Array(1, {MeteringRectangle(0, 0, 0, 0, 0)})
    mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AF_REGIONS, zeroMeteringRectangle)
    mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_REGIONS, zeroMeteringRectangle)
  }

  override fun getMaxZoom(): Float {
    return mCameraCharacteristics?.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM) ?: 0.0f
  }

  private fun isExposureCompensationSupported(): Boolean {
    // Turn off exposure compensation for Nexus 6 on L (API level 21)
    // because the bug in framework b/19219128.
    if ("motorola".equals(Build.MANUFACTURER, ignoreCase = true)
        && "shamu".equals(Build.DEVICE, ignoreCase = true) && Build.VERSION.SDK_INT == Build.VERSION_CODES.LOLLIPOP) {
      return false
    }
    val compensationRange = mCameraCharacteristics?.get<Range<Int>>(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE)
    return compensationRange != null && (compensationRange.lower != 0 || compensationRange.upper != 0)
  }

  private fun isAutoFocusSupported(): Boolean {
    val maxAfRegions = mCameraCharacteristics?.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF)
    // Auto-Focus is supported if the device supports one or more AF regions
    return maxAfRegions != null && maxAfRegions > 0
  }

  private fun isAutoExposureSupported(): Boolean {
    val maxAeRegions = mCameraCharacteristics?.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AE)
    // Auto-Exposure is supported if the device supports one or more AE regions
    return maxAeRegions != null && maxAeRegions > 0
  }

  fun isFlashSupported(): Boolean {
    val isSupportFlash = mCameraCharacteristics?.get(CameraCharacteristics.FLASH_INFO_AVAILABLE)
    return isSupportFlash != null && isSupportFlash
  }

  override fun getMinIso(): Int {
    val compensationRange = mCameraCharacteristics?.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE)
    return if (compensationRange == null) {
      0
    } else compensationRange.lower
  }

  override fun getMaxIso(): Int {
    val compensationRange = mCameraCharacteristics?.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE)
    return if (compensationRange == null) {
      0
    } else compensationRange.upper
  }

  override fun getCurrentIso(): Int {
    if (!isExposureCompensationSupported()) {
      return 0
    }
    val compensationStep = mCameraCharacteristics?.get<Rational>(CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP) ?: return 0
    return compensationStep.numerator / compensationStep.denominator
  }

  override fun setTorch(torch: Torch) {
    if (!isFlashSupported()) {
      return
    }
    if (torch == Torch.OFF) {
      mPreviewRequestBuilder?.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF)
    } else {
      mPreviewRequestBuilder?.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
      mPreviewRequestBuilder?.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_TORCH)
    }
    mCaptureSession?.setRepeatingRequest(mPreviewRequestBuilder?.build(), object: CameraCaptureSession.CaptureCallback() {

    }, mBackgroundHandler)
  }

}
