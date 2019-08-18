package com.trinity.camera

import android.graphics.PointF
import android.graphics.SurfaceTexture
import com.trinity.record.PreviewResolution

interface Camera {

  fun start(surfaceTexture: SurfaceTexture, resolution: PreviewResolution): Boolean

  fun stop()

  fun isCameraOpened(): Boolean

  fun setFacing(facing: Facing)

  fun getFacing(): Facing

  fun getSupportAspectRatios(): Set<AspectRatio>

  fun setAspectRatio(ratio: AspectRatio): Boolean

  fun setFlash(flash: Flash)

  fun getFlash(): Flash

  fun setDisplayOrientation(displayOrientation: Int)

  fun getWidth(): Int

  fun getHeight(): Int

  fun focus(viewWidth: Int, viewHeight: Int, point: PointF)
}