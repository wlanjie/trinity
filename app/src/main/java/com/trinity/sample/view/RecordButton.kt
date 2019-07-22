package com.trinity.sample.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.os.Handler
import android.os.Message
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import com.trinity.sample.R

class RecordButton : View {

  private var paint: Paint? = null
  private var mOnGestureListener: OnGestureListener? = null

  private var downColor: Int = 0
  private var upColor: Int = 0

  private var slideDis: Float = 0.toFloat()

  private var radiusDis: Float = 0.toFloat()
  private var currentRadius: Float = 0.toFloat()
  private var downRadius: Float = 0.toFloat()
  private var upRadius: Float = 0.toFloat()

  private var strokeWidthDis: Float = 0.toFloat()
  private var currentStrokeWidth: Float = 0.toFloat()
  private var minStrokeWidth: Float = 0.toFloat()
  private var maxStrokeWidth: Float = 0.toFloat()

  private val mHandler = object : Handler() {
    override fun handleMessage(msg: Message) {
      mOnGestureListener?.onDown()
    }
  }

  private var isDown: Boolean = false
  private var downX: Float = 0.toFloat()
  private var downY: Float = 0.toFloat()

  internal var changeStrokeWidth: Boolean = false
  internal var isAdd: Boolean = false

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
    downColor = R.color.video_gray
    upColor = android.R.color.white
    paint = Paint()
    paint?.isAntiAlias = true//抗锯齿
    paint?.style = Paint.Style.STROKE//画笔属性是空心圆
    currentStrokeWidth = resources.getDimension(R.dimen.dp10)
    paint?.strokeWidth = currentStrokeWidth//设置画笔粗细

    slideDis = resources.getDimension(R.dimen.dp10)
    radiusDis = resources.getDimension(R.dimen.dp3)
    strokeWidthDis = resources.getDimension(R.dimen.dp1) / 4

    minStrokeWidth = currentStrokeWidth
    maxStrokeWidth = currentStrokeWidth * 2
  }

  override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
    super.onLayout(changed, left, top, right, bottom)

    if (downRadius == 0f) {
      downRadius = width * 0.5f - currentStrokeWidth
      upRadius = width * 0.3f - currentStrokeWidth
    }
  }

  override fun onTouchEvent(event: MotionEvent): Boolean {

    when (event.action) {
      MotionEvent.ACTION_DOWN -> {
        val parent = parent as ViewGroup
        parent.requestDisallowInterceptTouchEvent(true)
        downX = event.rawX
        downY = event.rawY

        mHandler.sendEmptyMessageDelayed(0, 100)

        isDown = true
        invalidate()
      }
      MotionEvent.ACTION_MOVE -> {
      }
      MotionEvent.ACTION_UP -> {
        val parent1 = parent as ViewGroup
        parent1.requestDisallowInterceptTouchEvent(false)
        val upX = event.rawX
        val upY = event.rawY
        if (mHandler.hasMessages(0)) {
          if (Math.abs(upX - downX) < slideDis && Math.abs(upY - downY) < slideDis) {
            mOnGestureListener?.onClick()
          }
        } else {
          mOnGestureListener?.onUp()
        }
        initState()
      }
    }
    return true
  }

  fun initState() {
    isDown = false
    mHandler.removeMessages(0)
    invalidate()
  }

  fun setOnGestureListener(listener: OnGestureListener) {
    this.mOnGestureListener = listener
  }

  interface OnGestureListener {
    fun onDown()

    fun onUp()

    fun onClick()
  }

  override fun onDraw(canvas: Canvas) {
    super.onDraw(canvas)

    if (isDown) {
      paint?.color = ContextCompat.getColor(context, downColor)
      if (changeStrokeWidth) {
        if (isAdd) {
          currentStrokeWidth += strokeWidthDis
          if (currentStrokeWidth > maxStrokeWidth) isAdd = false
        } else {
          currentStrokeWidth -= strokeWidthDis
          if (currentStrokeWidth < minStrokeWidth) isAdd = true
        }
        paint?.strokeWidth = currentStrokeWidth
        currentRadius = width * 0.5f - currentStrokeWidth
      } else {
        if (currentRadius < downRadius) {
          currentRadius += radiusDis
        } else if (currentRadius >= downRadius) {
          currentRadius = downRadius
          isAdd = true
          changeStrokeWidth = true
        }
      }
      canvas.drawCircle(width / 2f, height / 2f, currentRadius, paint)
      invalidate()
    } else {
      changeStrokeWidth = false
      currentStrokeWidth = minStrokeWidth
      paint?.strokeWidth = currentStrokeWidth
      paint?.color = ContextCompat.getColor(context, upColor)
      if (currentRadius > upRadius) {
        currentRadius -= radiusDis
        invalidate()
      } else if (currentRadius < upRadius) {
        currentRadius = upRadius
        invalidate()
      }
      canvas.drawCircle(width / 2f, height / 2f, currentRadius, paint)
    }
  }
}
