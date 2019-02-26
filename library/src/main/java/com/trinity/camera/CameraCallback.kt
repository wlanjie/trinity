package com.trinity.camera

/**
 * Created by wlanjie on 2019/2/22.
 */

interface CameraCallback {

  fun onPreviewCallback(data: ByteArray, width: Int, height: Int, format: PreviewFormat)

  fun onPreviewSizeSelected(sizes: List<Size>): Size

  fun onPreviewFpsSelected(fpsRange: List<IntArray>): IntArray

  fun onFocusModeSelected(modes: List<String>): String

  fun onCameraFeatureSupported(autoExposureLockSupported: Boolean,
                               autoWhiteBalanceLockSupported: Boolean,
                               smoothZoomSupported: Boolean,
                               videoSnapshotSupported: Boolean,
                               videoStabilizationSupported: Boolean,
                               zoomSupported: Boolean,
                               focusSupported: Boolean,
                               flashSupported: Boolean)

  fun onCameraOpened(previewWidth: Int, previewHeight: Int)

  fun onCameraClosed()
}