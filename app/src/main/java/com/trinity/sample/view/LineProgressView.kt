package com.trinity.sample.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.util.TypedValue
import android.view.View
import androidx.core.content.ContextCompat
import com.trinity.sample.R

import java.util.ArrayList

class LineProgressView : View {

  private val paint = Paint()
  private val durations = mutableListOf<Int>()
  private var maxDuration = 0
  private var currentDuration = 0
  private var roundRectF = RectF()

  constructor(context: Context) : super(context) {
    init()
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init()
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    init()
  }

  private fun init() {
    paint.isAntiAlias = true
  }

  fun setMaxDuration(maxDuration: Int) {
    this.maxDuration = maxDuration
  }

  private fun addDuration(duration: Int) {
    durations.add(duration)
  }

  fun stop() {
    durations.add(currentDuration)
    currentDuration = 0
    invalidate()
  }

  fun setDuration(duration: Int) {
    currentDuration = duration
    invalidate()
  }

  override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
    super.onLayout(changed, left, top, right, bottom)
    paint.strokeWidth = height.toFloat()
    roundRectF = RectF(0f, 0f, width.toFloat(), TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 6.0f, resources.displayMetrics))
  }

  override fun onDraw(canvas: Canvas) {
    super.onDraw(canvas)

    paint.color = Color.parseColor("#80FF9800")
//    canvas.drawLine(0f, 0f, width.toFloat(), 0f, paint)
    canvas.drawRoundRect(roundRectF, 100.0f, 100.0f, paint)

    paint.color = Color.WHITE
    var current = 0
    durations.forEach {
      current += it
    }
    canvas.drawLine(0f, 0f, width.toFloat() * (current + currentDuration) / maxDuration, 0f, paint)

    paint.color = ContextCompat.getColor(context, R.color.colorPrimaryDark)
    var currentPoint = 0f
    durations.forEach {
      currentPoint += it
      canvas.drawLine(width.toFloat() * currentPoint/ maxDuration, 0f, width.toFloat() * currentPoint / maxDuration + roundRectF.bottom, 0f, paint)
    }
  }

  fun remove() {
    if (durations.isNotEmpty()) {
      durations.removeAt(durations.size - 1)
      invalidate()
    }
  }
}
