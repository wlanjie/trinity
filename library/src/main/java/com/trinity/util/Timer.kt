package com.trinity.util

import android.os.Handler
import android.os.HandlerThread
import android.os.Process
import android.os.SystemClock

class Timer(private val mListener: OnTimerListener, private var mUpdateInterval: Int) {

  private var mHandlerThread: HandlerThread? = null
  private var mHandler: Handler? = null
  private var mStartTime: Long = 0
  var duration: Int = 0

  private val mRunnable = object : Runnable {
    override fun run() {
      Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND)
      if (mStartTime == 0L) {
        mStartTime = SystemClock.elapsedRealtime()
      }
      val time = SystemClock.elapsedRealtime() - mStartTime
      if (time >= duration) {
        mListener.update(this@Timer, duration)
        mListener.end(this@Timer)
        return
      } else {
        mListener.update(this@Timer, time.toInt())
      }

      mHandler?.postDelayed(this, mUpdateInterval.toLong())
    }
  }

  interface OnTimerListener {
    // called for interval update
    fun update(timer: Timer, elapsedTime: Int)

    // called when the timer ends
    fun end(timer: Timer)
  }

  init {
    mHandlerThread = HandlerThread("time")
    mHandlerThread?.start()
    mHandler = Handler(mHandlerThread!!.looper)
  }

  fun setUpdateInterval(updateInterval: Int) {
    mUpdateInterval = updateInterval
  }

  fun start(duration: Int) {
    this.duration = duration
    mStartTime = 0
    stop()
    mHandler?.postDelayed(mRunnable, 0)
  }

  fun stop() {
    mStartTime = 0
    mHandler?.removeCallbacks(mRunnable)
  }

  fun release() {
    mHandlerThread?.looper?.quit()
    mHandlerThread?.quit()
    mHandlerThread = null
    mHandler?.removeCallbacks(mRunnable)
    mHandler = null
  }
}
