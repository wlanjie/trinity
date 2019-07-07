package com.trinity.sample.view

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.view.animation.AccelerateDecelerateInterpolator
import androidx.interpolator.view.animation.LinearOutSlowInInterpolator
import com.trinity.sample.R

class RecordButton @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) : View(context, attrs, defStyleAttr) {

  interface ActionListener {
    fun onStartRecord()

    fun onEndRecord()

    fun onDurationTooShortError()

    fun onSingleTap()

    fun onCancelled()
  }

  var actionListener: ActionListener? = null

  private var recordingColor: Int = Color.CYAN

  private var borderWidth: Float = context.resources.getDimension(R.dimen.cvb_border_width)

  private var isRecording: Boolean = false

  private var startRecordTime: Long = 0

  private var endRecordTime: Long = 0

  private var innerCircleMaxSize: Float = 0f

  private var innerCircleMinSize: Float = 0f

  private var innerCircleCurrentSize: Float = 0f

  private var outerCircleMaxSize: Float = 0f

  private var outerCircleMinSize: Float = 0f

  private var outerCircleCurrentSize: Float = 0f

  private var enableVideoRecording: Boolean = true

  private var enablePhotoTaking: Boolean = true

  private var videoDurationInMillis: Long = VIDEO_DURATION

  private var innerCirclePaint = Paint().apply {
    isAntiAlias = true
    style = Paint.Style.FILL
    color = Color.WHITE
  }

  private var outerCirclePaint = Paint().apply {
    isAntiAlias = true
    style = Paint.Style.FILL
    color = Color.WHITE
    alpha = 100
  }

  private var outerCircleBorderPaint = Paint().apply {
    isAntiAlias = true
    isDither = true
    color = recordingColor
    strokeWidth = borderWidth
    style = Paint.Style.STROKE
    strokeCap = Paint.Cap.ROUND
    strokeJoin = Paint.Join.ROUND
    pathEffect = CornerPathEffect(30f)
  }

  private var outerCircleBorderRect = RectF()

  private var outerCircleValueAnimator = ValueAnimator.ofFloat().apply {
    interpolator = LinearOutSlowInInterpolator()
    duration = MINIMUM_VIDEO_DURATION_MILLIS
    addUpdateListener {
      outerCircleCurrentSize = it.animatedValue as Float
      postInvalidate()
    }
  }

  private var outerCircleBorderValueAnimator = ValueAnimator.ofInt(0, videoDurationInMillis.toInt()).apply {
    interpolator = AccelerateDecelerateInterpolator()
    duration = videoDurationInMillis
    addUpdateListener {
      if ((it.animatedValue as Int) == videoDurationInMillis.toInt()) {
        onLongPressEnd()
      }
      postInvalidate()
    }
  }

  private var alphaAnimator = ValueAnimator.ofFloat(0f, 1f).apply {
    interpolator = AccelerateDecelerateInterpolator()
    duration = 400L
    addUpdateListener { outerCircleBorderPaint.alpha = ((it.animatedValue as Float) * 255.999).toInt() }
  }

  private var innerCircleSingleTapValueAnimator = ValueAnimator.ofFloat().apply {
    interpolator = AccelerateDecelerateInterpolator()
    duration = 300L
    addUpdateListener {
      innerCircleCurrentSize = it.animatedValue as Float
      postInvalidate()
    }
  }

  private var innerCircleLongPressValueAnimator = ValueAnimator.ofFloat().apply {
    interpolator = LinearOutSlowInInterpolator()
    duration = MINIMUM_VIDEO_DURATION_MILLIS
    addUpdateListener {
      innerCircleCurrentSize = it.animatedValue as Float
      postInvalidate()
    }
  }

  private val gestureDetector = GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {
    override fun onLongPress(e: MotionEvent) {
      if (enableVideoRecording) {
        onLongPressStart()
      }
    }

    override fun onSingleTapUp(e: MotionEvent?): Boolean {
      if (enablePhotoTaking) {
        onSingleTap()
        return true
      }
      return super.onSingleTapUp(e)
    }
  })

  init {
    val typedArray = context.theme.obtainStyledAttributes(attrs, R.styleable.RecordButton, defStyleAttr, defStyleAttr)
    recordingColor = typedArray.getColor(R.styleable.RecordButton_cvb_recording_color, Color.WHITE)
    outerCircleBorderPaint.color = recordingColor
    typedArray.recycle()
  }

  override fun onTouchEvent(event: MotionEvent?): Boolean {
    val detectedUp = event!!.action == MotionEvent.ACTION_UP
    return if (!gestureDetector.onTouchEvent(event) && detectedUp && enableVideoRecording) {
      onLongPressEnd()
      return true
    } else true
  }

  fun cancelRecording() {
    if (isRecording.not()) {
      return
    }

    isRecording = false
    endRecordTime = System.currentTimeMillis()

    innerCircleLongPressValueAnimator.setFloatValues(innerCircleCurrentSize, innerCircleMaxSize)
    innerCircleLongPressValueAnimator.start()

    outerCircleValueAnimator.setFloatValues(outerCircleCurrentSize, outerCircleMinSize)
    outerCircleValueAnimator.start()

    outerCircleBorderValueAnimator.cancel()

    alphaAnimator.setFloatValues(1f, 0f)
    alphaAnimator.start()

    actionListener?.onCancelled()

    resetRecordingValues()
  }

  fun enableVideoRecording(enableVideoRecording: Boolean) {
    this.enableVideoRecording = enableVideoRecording
  }

  fun enablePhotoTaking(enablePhotoTaking: Boolean) {
    this.enablePhotoTaking = enablePhotoTaking
  }

  fun setVideoDuration(durationInMillis: Long) {
    this.videoDurationInMillis = durationInMillis
    with(outerCircleBorderValueAnimator) {
      setIntValues(0, durationInMillis.toInt())
      duration = durationInMillis
    }
  }

  private fun onLongPressStart() {
    isRecording = true
    actionListener?.onStartRecord()
    startRecordTime = System.currentTimeMillis()

    innerCircleLongPressValueAnimator.setFloatValues(innerCircleCurrentSize, innerCircleMinSize * 1.75F)
    innerCircleLongPressValueAnimator.start()

    outerCircleValueAnimator.setFloatValues(outerCircleCurrentSize, outerCircleMaxSize)
    outerCircleValueAnimator.start()

    outerCircleBorderValueAnimator.start()

    alphaAnimator.setFloatValues(0f, 1f)
    alphaAnimator.start()
  }

  private fun onLongPressEnd() {
    if (isRecording.not()) {
      return
    }

    isRecording = false
    endRecordTime = System.currentTimeMillis()

    innerCircleLongPressValueAnimator.setFloatValues(innerCircleCurrentSize, innerCircleMaxSize)
    innerCircleLongPressValueAnimator.start()

    outerCircleValueAnimator.setFloatValues(outerCircleCurrentSize, outerCircleMinSize)
    outerCircleValueAnimator.start()

    outerCircleBorderValueAnimator.cancel()

    alphaAnimator.setFloatValues(1f, 0f)
    alphaAnimator.start()

    if (isRecordTooShort(startRecordTime, endRecordTime, MINIMUM_VIDEO_DURATION_MILLIS)) {
      actionListener?.onDurationTooShortError()
    } else if (isRecording) {
      actionListener?.onEndRecord()
    }

    resetRecordingValues()
  }

  private fun onSingleTap() {
    actionListener?.onSingleTap()
    innerCircleSingleTapValueAnimator.start()
  }

  private fun isRecordTooShort(startMillis: Long, endMillis: Long, minimumMillisRange: Long): Boolean {
    return endMillis - startMillis < minimumMillisRange
  }

  override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
    super.onSizeChanged(w, h, oldw, oldh)
    val minSide = Math.min(w, h)

    innerCircleMaxSize = minSide.toFloat() / 2
    innerCircleMinSize = minSide.toFloat() / 4
    innerCircleCurrentSize = innerCircleMaxSize

    innerCircleSingleTapValueAnimator.setFloatValues(innerCircleMaxSize, innerCircleMinSize, innerCircleMaxSize)

    outerCircleMaxSize = minSide.toFloat()
    outerCircleMinSize = minSide.toFloat() / 1.5f
    outerCircleCurrentSize = outerCircleMinSize

    outerCircleBorderRect.set(
      borderWidth / 2,
      borderWidth / 2,
      outerCircleMaxSize - borderWidth / 2f,
      outerCircleMaxSize - borderWidth / 2)

    outerCircleValueAnimator.setFloatValues(outerCircleMinSize, outerCircleMaxSize)

  }

  override fun onDraw(canvas: Canvas?) {
    super.onDraw(canvas)

    if (canvas == null) {
      return
    }

    canvas.drawCircle(outerCircleMaxSize / 2, outerCircleMaxSize / 2, innerCircleCurrentSize / 2, innerCirclePaint)
    canvas.drawCircle(outerCircleMaxSize / 2, outerCircleMaxSize / 2, outerCircleCurrentSize / 2, outerCirclePaint)

    if (isRecording) {
      canvas.drawArc(outerCircleBorderRect, -90f, calculateCurrentAngle(), false, outerCircleBorderPaint)
    }
  }

  private fun resetRecordingValues() {
    startRecordTime = 0
    endRecordTime = 0
  }

  private fun calculateCurrentAngle(): Float {
    System.currentTimeMillis()
    val millisPassed = System.currentTimeMillis() - startRecordTime
    return millisPassed * 360f / videoDurationInMillis
  }

  companion object {
    private const val MINIMUM_VIDEO_DURATION_MILLIS = 300L
    private const val VIDEO_DURATION = 10000L
  }
}
