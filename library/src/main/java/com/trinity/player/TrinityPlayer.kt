package com.trinity.player

import android.view.Surface

open class TrinityPlayer : MediaCodecDecoderLifeCycle() {

  private var mHandle: Long = 0
  private var initializedCallback: OnInitializedCallback? = null

  fun init(
    srcFilenameParam: String, surface: Surface, width: Int, height: Int,
    onInitializedCallback: OnInitializedCallback
  ) {
    mHandle = create()
    init(
      srcFilenameParam, intArrayOf(-1, -1, -1), -1, 0.5f, 1.0f, surface, width, height,
      onInitializedCallback
    )
  }

  fun init(
    srcFilenameParam: String, max_analyze_duration: IntArray, probesize: Int,
    minBufferedDuration: Float, maxBufferedDuration: Float, surface: Surface, width: Int,
    height: Int, onInitializedCallback: OnInitializedCallback
  ) {
    initializedCallback = onInitializedCallback

    prepare(
      mHandle,
      srcFilenameParam, max_analyze_duration, max_analyze_duration.size, probesize, true, minBufferedDuration,
      maxBufferedDuration, width, height, surface
    )
  }

  fun onInitializedFromNative(initCode: Boolean) {
    if (initializedCallback != null) {
      val onInitialStatus =
        if (initCode) OnInitializedCallback.OnInitialStatus.CONNECT_SUCESS else OnInitializedCallback.OnInitialStatus.CONNECT_FAILED
      initializedCallback!!.onInitialized(onInitialStatus)
    }
  }

  private external fun create(): Long

  fun onSurfaceCreated(surface: Surface) {
    onSurfaceCreated(mHandle, surface)
  }

  private external fun onSurfaceCreated(handle: Long, surface: Surface)

  fun onSurfaceDestroyed(surface: Surface) {
    onSurfaceDestroyed(mHandle, surface)
  }

  private external fun onSurfaceDestroyed(handle: Long, surface: Surface)

  fun getBufferedProgress(): Float {
    return getBufferedProgress(mHandle)
  }

  /**
   * 获得缓冲进度 返回秒数（单位秒 但是小数点后有3位 精确到毫秒）
   */
  private external fun getBufferedProgress(handle: Long): Float

  fun getPlayProgress(): Float {
    return getPlayProgress(mHandle)
  }

  /**
   * 获得播放进度（单位秒 但是小数点后有3位 精确到毫秒）
   */
  private external fun getPlayProgress(handle: Long): Float

  private external fun prepare(
    handle: Long,
    srcFilenameParam: String,
    maxAnalyzeDurations: IntArray,
    size: Int,
    probeSize: Int,
    fpsProbeSizeConfigured: Boolean,
    minBufferedDuration: Float,
    maxBufferedDuration: Float,
    width: Int,
    height: Int,
    surface: Surface
  ): Boolean

  fun pause() {
    pause(mHandle)
  }

  private external fun pause(handle: Long)

  fun play() {
    play(mHandle)
  }

  private external fun play(handle: Long)

  fun stopPlay() {
    object : Thread() {
      override fun run() {
        stop(mHandle)
        release(mHandle)
      }
    }.start()
  }

  private external fun stop(handle: Long)

  fun seekToPosition(position: Float) {
    seekToPosition(mHandle, position)
  }

  /**
   * 跳转到某一个位置
   */
  private external fun seekToPosition(handle: Long, position: Float)

  fun seekCurrent(position: Float) {
    seekCurrent(mHandle, position)
  }

  /**
   * 只做seek操作，seek到指定位置
   */
  private external fun seekCurrent(handle: Long, position: Float)

  fun beforeSeekCurrent() {
    beforeSeekCurrent(mHandle)
  }

  private external fun beforeSeekCurrent(handle: Long)

  fun afterSeekCurrent() {
    afterSeekCurrent(mHandle)
  }

  private external fun afterSeekCurrent(handle: Long)

  fun resetRenderSize(left: Int, top: Int, width: Int, height: Int) {
    resetRenderSize(mHandle, left, top, width, height)
  }

  private external fun resetRenderSize(handle: Long, left: Int, top: Int, width: Int, height: Int)
  private external fun release(handle: Long)

  open fun onCompletion() {
  }

  open fun videoDecodeException() {
  }

  open fun viewStreamMetaCallback(width: Int, height: Int, duration: Float) {
  }

  fun showLoadingDialog() {
  }

  fun hideLoadingDialog() {
  }


}
