package com.trinity.camera

import android.graphics.SurfaceTexture

interface Camera {

  fun start(surfaceTexture: SurfaceTexture): Boolean

  fun stop()

  fun isCameraOpened(): Boolean

  fun setFacing(facing: Int)

  fun getFacing(): Int

  fun getSupportAspectRatios(): Set<AspectRatio>

  fun setAspectRatio(ratio: AspectRatio): Boolean

  fun setFlash(flash: Flash)

  fun getFlash(): Flash

  fun setDisplayOrientation(displayOrientation: Int)

  fun getWidth(): Int

  fun getHeight(): Int
}