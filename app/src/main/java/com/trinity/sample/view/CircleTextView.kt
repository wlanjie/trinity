package com.trinity.sample.view

import android.content.Context
import android.content.res.TypedArray
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.ColorFilter
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.RectF
import android.graphics.drawable.Drawable
import android.os.Build
import android.util.AttributeSet
import android.widget.TextView

import com.trinity.sample.R

class CircleTextView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) : TextView(context, attrs, defStyleAttr) {
  private val backgroundColor: Int

  init {
    val typedArray = context.obtainStyledAttributes(attrs, R.styleable.CircleTextView, defStyleAttr, 0)
    backgroundColor = typedArray.getColor(R.styleable.CircleTextView_backgroundColor, Color.TRANSPARENT)
    typedArray.recycle()
  }

  override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      background = SemiCircleRectDrawable()
    } else {
      setBackgroundDrawable(SemiCircleRectDrawable())
    }
  }

  internal inner class SemiCircleRectDrawable : Drawable() {
    private val mPaint: Paint
    private var rectF: RectF? = null

    init {
      mPaint = Paint()
      mPaint.isAntiAlias = true
      mPaint.color = backgroundColor
    }

    override fun setBounds(left: Int, top: Int, right: Int, bottom: Int) {
      super.setBounds(left, top, right, bottom)
      if (rectF == null) {
        rectF = RectF(left.toFloat(), top.toFloat(), right.toFloat(), bottom.toFloat())
      } else {
        rectF!!.set(left.toFloat(), top.toFloat(), right.toFloat(), bottom.toFloat())
      }
    }

    override fun draw(canvas: Canvas) {
      var R = rectF!!.bottom / 2
      if (rectF!!.right < rectF!!.bottom) {
        R = rectF!!.right / 2
      }
      canvas.drawRoundRect(rectF!!, R, R, mPaint)
    }

    override fun setAlpha(alpha: Int) {
      mPaint.alpha = alpha
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
      mPaint.colorFilter = colorFilter
    }

    override fun getOpacity(): Int {
      return PixelFormat.OPAQUE
    }
  }

  companion object {
    private val TAG = "SemiCircleRectView"
  }
}