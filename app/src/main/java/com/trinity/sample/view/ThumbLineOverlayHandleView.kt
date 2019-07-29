package com.trinity.sample.view

import android.view.MotionEvent
import android.view.View
import androidx.core.view.MotionEventCompat

class ThumbLineOverlayHandleView(val view: View?             //对应的View
                                 , duration: Long) : View.OnTouchListener {
  var duration: Long = 0
    private set         //所处的时长
  private var mPositionChangeListener: OnPositionChangeListener? = null
  private var mStartX: Float = 0.toFloat()

  internal interface OnPositionChangeListener {
    fun onPositionChanged(distance: Float)

    fun onChangeComplete()
  }

  init {
    if (this.view != null) {
      this.view.setOnTouchListener(this)
    }
    this.duration = duration
  }

  override fun onTouch(v: View, event: MotionEvent): Boolean {
    val actionMasked = MotionEventCompat.getActionMasked(event)
    when (actionMasked) {
      MotionEvent.ACTION_DOWN -> mStartX = event.rawX
      MotionEvent.ACTION_MOVE -> {
        val dx = event.rawX - mStartX
        mStartX = event.rawX
        if (mPositionChangeListener != null) {
          mPositionChangeListener!!.onPositionChanged(dx)
        }
      }
      MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
        if (mPositionChangeListener != null) {
          mPositionChangeListener!!.onChangeComplete()
        }
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
    if (view != null) {
      view.visibility = View.VISIBLE
    }
  }

  fun fix() {
    if (view != null) {
      view.visibility = View.INVISIBLE
    }
  }


  fun changeDuration(duration: Long) {
    this.duration += duration
  }
}

