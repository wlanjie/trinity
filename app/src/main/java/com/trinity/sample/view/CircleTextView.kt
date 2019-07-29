package com.trinity.sample.view

import android.R.attr.textColor
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Rect
import android.util.AttributeSet
import android.widget.TextView
import com.trinity.sample.R

class CircleTextView  : TextView {
  private var mPaint: Paint? = null
  private var defaultColor: Int = 0

  constructor(context: Context) : super(context) {
    init(context, null, 0)
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init(context, attrs, 0)
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    init(context, attrs, defStyleAttr)
  }

  private fun init(context: Context, attrs: AttributeSet?, defStyleAttr: Int) {
    val typedArray = context.obtainStyledAttributes(attrs, R.styleable.CircleTextView)
    defaultColor = typedArray.getInt(R.styleable.CircleTextView_backgroundColor, -1)
    typedArray.recycle()

    mPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    mPaint?.color = defaultColor
    mPaint?.style = Paint.Style.FILL
  }

  fun setColor(color: Int) {
    mPaint?.color = color
    invalidate()
  }

  override fun onDraw(canvas: Canvas) {
    canvas.drawCircle((measuredWidth / 2).toFloat(), (measuredHeight / 2).toFloat(), (measuredWidth / 2).toFloat(), mPaint)
    super.onDraw(canvas)
  }

}