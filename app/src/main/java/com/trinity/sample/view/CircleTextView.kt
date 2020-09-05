package com.trinity.sample.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import androidx.appcompat.widget.AppCompatTextView
import com.trinity.sample.R

class CircleTextView  : AppCompatTextView {
  private var mPaint: Paint? = null
  private var defaultColor: Int = 0

  constructor(context: Context) : super(context) {
    init(context, null)
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init(context, attrs)
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    init(context, attrs)
  }

  private fun init(context: Context, attrs: AttributeSet?) {
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
    mPaint?.let {
      canvas.drawCircle((measuredWidth / 2).toFloat(), (measuredHeight / 2).toFloat(), (measuredWidth / 2).toFloat(), it)
    }
    super.onDraw(canvas)
  }

}