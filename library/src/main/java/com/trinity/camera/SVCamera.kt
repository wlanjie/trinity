package com.trinity.camera

import android.graphics.SurfaceTexture

/**
 * Created by wlanjie on 2017/12/20.
 */

interface SVCamera {

  @Throws(IllegalArgumentException::class)
  fun configCamera(facing: Facing): CameraConfigInfo

  /**
   * 开房预览
   * @return 打开摄像头是否成功
   */
  fun start(surfaceTexture: SurfaceTexture): Boolean

  /**
   * 停止预览
   */
  fun stop()

  /**
   * 切换摄像头 FRONT 前置摄像头 BACK 后置摄像头
   * @param facing 摄像头id
   */
  fun setFacing(facing: Facing)

  /**
   * 获取当前摄像头id
   * @return 摄像头id
   */
  fun getFacing(): Facing

  /**
   * 摄像头是否打开j
   * @return 摄像是否打开
   */
  fun isCameraOpened(): Boolean

  /**
   * set view width and height
   */
  fun setViewSize(width: Int, height: Int)

  /**
   * enable/disabled auto focus
   */
  fun setAutoFocus(enable: Boolean)

  /**
   * 设置预览缩放
   * @param zoom 缩放值
   */
  @Throws(IllegalStateException::class)
  fun setZoom(zoom: Int)

  /**
   * 设置爆光度
   * @param iso 爆光值
   */
  @Throws(IllegalArgumentException::class)
  fun setIso(iso: Int)

  /**
   * 设置手动对焦
   * @param x
   */
  fun focus(x: Float, y: Float)

  /**
   * 获取最大缩放值
   * @return 缩放最大值
   */
  fun getMaxZoom(): Float

  /**
   * 获取最小爆光度
   * @return 爆光最小值
   */
  fun getMinIso(): Int

  /**
   * 获取最大爆光度
   * @return 爆光最大值
   */
  fun getMaxIso(): Int

  /**
   * 获取当前爆光值
   * @return 当前爆光值
   */
  fun getCurrentIso(): Int

  /**
   * 打开闪光灯
   * @param torch OFF 关闭闪光灯, ON 打开闪光灯
   */
  fun setTorch(torch: Torch)
}