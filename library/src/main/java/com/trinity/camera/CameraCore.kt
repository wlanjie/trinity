package com.trinity.camera

import android.content.Context
import android.graphics.SurfaceTexture
import android.os.Build

object CameraCore {

  /**
   * @param context android 上下文
   * @param cameraCallback 相机相关回调
   * @param surfaceTexture 预览SurfaceTexture
   * @return 相机接口
   */
  fun createCamera(context: Context, cameraCallback: CameraCallback, surfaceTexture: SurfaceTexture): TCamera {
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      Camera2(context, cameraCallback)
    } else {
      Camera1(context, cameraCallback)
    }
  }

  /**
   * create camera1
   */
  fun createCamera1(context: Context, cameraCallback: CameraCallback, surfaceTexture: SurfaceTexture): TCamera {
    return Camera1(context, cameraCallback)
  }
}