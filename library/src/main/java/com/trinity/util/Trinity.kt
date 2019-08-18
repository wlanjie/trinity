package com.trinity.util

import android.os.Handler
import android.os.Looper

/**
 * Created by wlanjie on 2019-07-31
 */
object Trinity {

  private val mHandler = Handler(Looper.getMainLooper())

  fun <T> callback(body: () -> T) {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      body()
    } else {
      mHandler.post {
        body()
      }
    }
  }
}