package com.trinity

import android.app.Activity
import android.graphics.SurfaceTexture
import android.hardware.Camera
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.trinity.camera.*
import java.lang.Exception

/**
 * Create by wlanjie on 2019/2/20-下午8:58
 */
class Record(activity: Activity) : SurfaceHolder.Callback, CameraCallback, TrinityCamera.ChangbaVideoCameraCallback {
    override fun onPermissionDismiss(tip: String?) {
    }

    override fun notifyFrameAvailable() {
        onFrameAvailable(mId)
    }

    override fun updateTexMatrix(texMatrix: FloatArray?) {
    }

    override fun onPreviewCallback(data: ByteArray, width: Int, height: Int, format: PreviewFormat) {
    }

    override fun onPreviewSizeSelected(sizes: List<Size>): Size {
        return Size(1280, 720)
    }

    override fun onPreviewFpsSelected(fpsRange: List<IntArray>): IntArray {
        return fpsRange[0]
    }

    override fun onFocusModeSelected(modes: List<String>): String {
        return modes[0]
    }

    override fun onCameraFeatureSupported(
        autoExposureLockSupported: Boolean,
        autoWhiteBalanceLockSupported: Boolean,
        smoothZoomSupported: Boolean,
        videoSnapshotSupported: Boolean,
        videoStabilizationSupported: Boolean,
        zoomSupported: Boolean,
        focusSupported: Boolean,
        flashSupported: Boolean
    ) {
    }

    override fun onCameraOpened(previewWidth: Int, previewHeight: Int) {
    }

    override fun onCameraClosed() {
    }

    private var mPreviewing = false
    private var mFirst = true
    private var mSurfaceCreate = false
    private var mStop = false
    private var mCamera = TrinityCamera(activity)
    private var mView: SurfaceView ?= null
    private var mCameraFacingId = Camera.CameraInfo.CAMERA_FACING_FRONT
    private var mId: Long = 0
    private var mCameraConfigInfo: CameraConfigInfo ?= null
    private var mSurfaceTexture: SurfaceTexture ?= null

    init {
        mId = createNative()
        mCamera.setCallback(this)
    }

    fun setView(view: SurfaceView) {
        mView = view
        val holder = view.holder
        holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // 检查是否正在预览
        if (mPreviewing) {
            return
        }
        startPreview(holder.surface, mView?.width ?: 0, mView?.height ?: 0, mCameraFacingId)
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
        resetRenderSize(mId, width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
        if (mStop) {
            stopPreview()
        } else {
            destroyWindowSurface(mId)
        }
        mSurfaceCreate = false
    }

    private fun startPreview(surface: Surface, width: Int, height: Int, cameraFacingId: Int) {
        if (mFirst) {
            prepareEGLContext(mId, surface, width, height, cameraFacingId)
            mFirst = false
        } else {
            createWindowSurface(mId, surface)
        }
        mSurfaceCreate = true
    }

    private fun stopPreview() {
        destroyEGLContext(mId)
        mFirst = true
        mSurfaceCreate = false
        mStop = false
    }

    public fun stop() {
        mStop = true
    }

    private fun startPreviewFromNative(textureId: Int) {
//        mSurfaceTexture = SurfaceTexture(textureId)
//        mSurfaceTexture?.let {
//            mCamera.start(it)
//        }
        mCamera.setCameraPreviewTexture(textureId)
    }

    private fun configCameraFromNative(cameraFacingId: Int): CameraConfigInfo? {
        mCameraFacingId = cameraFacingId
        mCameraConfigInfo = mCamera.configCameraFromNative(cameraFacingId)
        return mCameraConfigInfo
//        return CameraConfigInfo(mCamera.getCameraDisplayOrientation(), 1280, 720, cameraFacingId)
    }

    private fun updateTexImageFromNative() {
//        try {
//            mSurfaceTexture?.updateTexImage()
//        } catch (e: Exception) {
//            e.printStackTrace()
//        }

        mCamera.updateTexImage()
    }

    private fun releaseCameraFromNative() {
        mCamera.releaseCamera()
    }

    private external fun createNative(): Long

    private external fun prepareEGLContext(id: Long, surface: Surface, width: Int, height: Int, cameraFacingId: Int)

    private external fun createWindowSurface(id: Long, surface: Surface)

    private external fun adaptiveVideoQuality(id: Long, maxBitRate: Int, avgBitRate: Int, fps: Int)

    private external fun hotConfigQuality(id: Long, maxBitRate: Int, avgBitRate: Int, fps: Int)

    private external fun resetRenderSize(id: Long, width: Int, height: Int)

    private external fun onFrameAvailable(id: Long)

    private external fun updateTextureMatrix(id: Long, textureMatrix: FloatArray)

    private external fun destroyWindowSurface(id: Long)

    private external fun destroyEGLContext(id: Long)

    private external fun releaseNative(id: Long)
}