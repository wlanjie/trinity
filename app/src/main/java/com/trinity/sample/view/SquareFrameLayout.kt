package com.trinity.sample.view

import android.content.Context
import android.util.AttributeSet
import android.widget.FrameLayout

class SquareFrameLayout : FrameLayout, SizeChangedNotifier {

  private var mOnSizeChangedListener: SizeChangedNotifier.Listener? = null

  constructor(context: Context) : super(context) {}

  constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {}

  constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {}


  override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
    val widthSize = View.MeasureSpec.getSize(widthMeasureSpec)
    val heightSize = View.MeasureSpec.getSize(heightMeasureSpec)

    if (widthSize == 0 && heightSize == 0) {
      // If there are no constraints on size, let FrameLayout measure
      super.onMeasure(widthMeasureSpec, heightMeasureSpec)

      // Now use the smallest of the measured dimensions for both dimensions
      val minSize = Math.min(measuredWidth, measuredHeight)
      setMeasuredDimension(minSize, minSize)
      return
    }

    val size: Int
    if (widthSize == 0 || heightSize == 0) {
      // If one of the dimensions has no restriction on size, set both dimensions to be the
      // on that does
      size = Math.max(widthSize, heightSize)
    } else {
      // Both dimensions have restrictions on size, set both dimensions to be the
      // smallest of the two
      size = Math.min(widthSize, heightSize)
    }

    val newMeasureSpec = View.MeasureSpec.makeMeasureSpec(size, View.MeasureSpec.EXACTLY)
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
