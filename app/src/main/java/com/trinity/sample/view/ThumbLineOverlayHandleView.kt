package com.trinity.sample.view

import android.annotation.SuppressLint
import android.view.MotionEvent
import android.view.View
import androidx.core.view.MotionEventCompat

class ThumbLineOverlayHandleView(val view: View?, duration: Long) : View.OnTouchListener {
  var duration: Long = 0
  private var mPositionChangeListener: OnPositionChangeListener? = null
  private var mStartX: Float = 0.toFloat()

  interface OnPositionChangeListener {
    fun onPositionChanged(distance: Float)
    fun onChangeComplete()
  }

  init {
    view?.setOnTouchListener(this)
    this.duration = duration
  }

  @SuppressLint("ClickableViewAccessibility")
  override fun onTouch(v: View, event: MotionEvent): Boolean {
    when (MotionEventCompat.getActionMasked(event)) {
      MotionEvent.ACTION_DOWN -> mStartX = event.rawX
      MotionEvent.ACTION_MOVE -> {
        val dx = event.rawX - mStartX
        mStartX = event.rawX
        mPositionChangeListener?.onPositionChanged(dx)
      }
      MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
        mPositionChangeListener?.onChangeComplete()
        mStartX = 0f
      }
      else -> mStartX = 0f
    }
    return true
  }

  fun setPositionChangeListener(positionChangeListener: OnPositionChangeListener) {
    mPositionChangeListener = positionChangeListener
  }

  fun active() {
    view?.visibility = View.VISIBLE
  }

  fun fix() {
    view?.visibility = View.INVISIBLE
  }


  fun changeDuration(duration: Long) {
    this.duration += duration
  }
}

