/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

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
