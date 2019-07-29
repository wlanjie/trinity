package com.trinity.sample.view

import android.content.Context
import android.util.AttributeSet
import android.widget.FrameLayout
import kotlin.math.max
import kotlin.math.min

class SquareFrameLayout : FrameLayout, SizeChangedNotifier {

  private var mOnSizeChangedListener: SizeChangedNotifier.Listener? = null

  constructor(context: Context) : super(context) {}

  constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {}

  constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {}


  override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
    val widthSize = MeasureSpec.getSize(widthMeasureSpec)
    val heightSize = MeasureSpec.getSize(heightMeasureSpec)

    if (widthSize == 0 && heightSize == 0) {
      // If there are no constraints on size, let FrameLayout measure
      super.onMeasure(widthMeasureSpec, heightMeasureSpec)

      // Now use the smallest of the measured dimensions for both dimensions
      val minSize = min(measuredWidth, measuredHeight)
      setMeasuredDimension(minSize, minSize)
      return
    }

    val size = if (widthSize == 0 || heightSize == 0) {
      max(widthSize, heightSize)
    } else {
      min(widthSize, heightSize)
    }

    val newMeasureSpec = MeasureSpec.makeMeasureSpec(size, MeasureSpec.EXACTLY)
    super.onMeasure(newMeasureSpec, newMeasureSpec)
  }

  override fun setOnSizeChangedListener(listener: SizeChangedNotifier.Listener) {
    mOnSizeChangedListener = listener
  }

  override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
    super.onSizeChanged(w, h, oldw, oldh)

    if (mOnSizeChangedListener != null) {
      mOnSizeChangedListener!!.onSizeChanged(this, w, h, oldw, oldh)
    }
  }
}
