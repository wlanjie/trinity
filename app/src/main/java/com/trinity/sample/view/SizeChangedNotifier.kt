package com.trinity.sample.view

import android.view.View

interface SizeChangedNotifier {

  interface Listener {
    fun onSizeChanged(view: View, w: Int, h: Int, oldw: Int, oldh: Int)
  }

  fun setOnSizeChangedListener(listener: Listener)

}
