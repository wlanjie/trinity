/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.trinity.sample.view

import android.content.Context
import android.graphics.Matrix
import android.graphics.Point
import android.util.AttributeSet
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import com.trinity.sample.R
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.sign
import kotlin.math.sqrt

abstract class AbstractPasteView : ViewGroup {

  companion object {

    /**
     * normalized 2d coordinates: x y: (0, 0) right bottom: (1, 1) object
     * coordinates: (0, 0) (1, 1)
     */
    private const val MODE_NORMALIZED = 1

    /**
     * device coordinates: x y: (- CanvasWidth / 2, - CanvasHeight / 2)
     * right bottom: (+ CanvasWidth / 2, + CanvasHeight / 2) object coordinates:
     * (- ContentWidth / 2, - ContentHeight / 2) (+ ContentWidth / 2, +
     * ContentHeight / 2)
     */
    private const val MODE_DEVICE = 2

    /**
     * viewport coordinates: x y: (- ViewportWidth / 2, - ViewportHeight /
     * 2) right bottom: (+ ViewportWidth / 2, + ViewportWidth / 2) object
     * coordinates: (- ContentWidth / 2, - ContentHeight / 2) (+ ContentWidth /
     * 2, + ContentHeight / 2)
     */
    private const val MODE_VIEWPORT = 3

    private val POINT_LIST = FloatArray(8)
  }

  private var mViewportWidth = 640
  private var mViewportHeight = 640
  private var mLayoutWidth = 0
  private var mLayoutHeight = 0
  private var mListener: OnChangeListener? = null
  protected val mTransform = Matrix()
  private var mTransformMode = MODE_DEVICE
  private val mInverseTransform = Matrix()
  private var mInverseTransformInvalidated = false
  protected val mMatrixUtil: MatrixUtil = MatrixUtil()

  constructor(context: Context) : super(context) {
    init()
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init()
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    init()
  }

  fun init() {
    post {
      val parent = parent as ViewGroup
      mLayoutWidth = parent.width
      mLayoutHeight = parent.height
    }
  }

  class LayoutParams : MarginLayoutParams {

    var gravity = 0

    constructor(c: Context, attrs: AttributeSet?) : super(c, attrs) {
      val ta = c.obtainStyledAttributes(attrs, R.styleable.EditOverlay_Layout)
      gravity = ta.getInteger( R.styleable.EditOverlay_Layout_android_layout_gravity,
          Gravity.LEFT or Gravity.TOP)
      ta.recycle()
    }

    constructor(width: Int, height: Int) : super(width, height)
  }

  private var isMirror = false

  fun getLayoutWidth(): Int {
    return mLayoutWidth
  }

  fun getLayoutHeight(): Int {
    return mLayoutHeight
  }

  fun setViewport(w: Int, h: Int) {
    mViewportWidth = w
    mViewportHeight = h
  }

  fun isMirror(): Boolean {
    return isMirror
  }

  fun setMirror(mirror: Boolean) {
    isMirror = mirror
  }

  fun setOnChangeListener(listener: OnChangeListener?) {
    mListener = listener
  }

  fun getScale(): FloatArray? {
    val scale = FloatArray(2)
    scale[0] = mMatrixUtil.scaleX
    scale[1] = mMatrixUtil.scaleY
    return scale
  }

  override fun getRotation(): Float {
    return mMatrixUtil.rotation
  }

  abstract fun getContentWidth(): Int

  abstract fun getContentHeight(): Int

  abstract fun setContentWidth(width: Int)

  abstract fun setContentHeight(height: Int)

  abstract fun getContentView(): View?

  open fun isCouldShowLabel(): Boolean {
    return false
  }

  open fun setShowTextLabel(isShow: Boolean) {}

  open fun getTextLabel(): View? {
    return null
  }

  open fun setEditCompleted(isEditCompleted: Boolean) {}

  protected fun validateTransform() {
    if (getContentWidth() == 0 || getContentHeight() == 0) {
      return
    }
    when (mTransformMode) {
      MODE_DEVICE -> return
      MODE_NORMALIZED -> {
        mTransform.preTranslate(0.5f, 0.5f)
        mTransform.preScale(1.0f / getContentWidth(), 1.0f / getContentHeight())
        mTransform.postTranslate(-0.5f, -0.5f)
        //Log.d("VALIDATE", "getWidth : " + getWidth() + " getHeight : " + getHeight());
        mTransform.postScale(width.toFloat(), height.toFloat())
      }
      MODE_VIEWPORT -> mTransform.postScale(width.toFloat() / mViewportWidth,
          height.toFloat() / mViewportHeight)
    }
    mTransformMode = MODE_DEVICE
  }

  override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
    validateTransform()
    onLayoutContent()
    val hw = getContentWidth() / 2.toFloat()
    val hh = getContentHeight() / 2.toFloat()

    // lt, rt, lb, rb
    POINT_LIST[0] = -hw
    POINT_LIST[1] = -hh
    POINT_LIST[2] = hw
    POINT_LIST[3] = -hh
    POINT_LIST[4] = -hw
    POINT_LIST[5] = hh
    POINT_LIST[6] = hw
    POINT_LIST[7] = hh
    val baseX = width / 2
    val baseY = height / 2
    val together = converge()
    together.mapPoints(POINT_LIST)
    var i = 0
    val count = childCount
    while (i < count) {
      val child = getChildAt(i)
      if (child === getContentView()) {
        i++
        continue
      }
      val gravity = (child.layoutParams as LayoutParams).gravity
      val ix = getPointFromMatrix(gravity)
      val centerX = POINT_LIST[ix].toInt()
      val centerY = POINT_LIST[ix + 1].toInt()
      val left = baseX + centerX
      val top = baseY + centerY
      val right = baseX + centerX
      val bottom = baseY + centerY
      val halfW = child.measuredWidth / 2
      val halfH = child.measuredHeight / 2
      val childLeft = left - halfW
      val childTop = top - halfH
      val childRight = right + halfW
      val childBottom = bottom + halfH
      child.layout(childLeft, childTop, childRight, childBottom)
      i++
    }
  }

  private fun getPointFromMatrix(gravity: Int): Int {
    var x = 0
    var y = 0
    when (gravity and Gravity.VERTICAL_GRAVITY_MASK) {
      Gravity.TOP -> y = 0
      Gravity.BOTTOM -> y = 1
    }
    when (gravity and Gravity.HORIZONTAL_GRAVITY_MASK) {
      Gravity.LEFT -> x = 0
      Gravity.RIGHT -> x = 1
    }
    return (x + y * 2) * 2
  }

  override fun generateLayoutParams(attrs: AttributeSet?): LayoutParams? {
    return LayoutParams(context, attrs)
  }

  private fun invalidateTransform() {
    mInverseTransformInvalidated = true
    validateTransform()
    requestLayout()
  }

  private fun onLayoutContent() {
    mMatrixUtil.decomposeTSR(mTransform)
    val hw: Float = mMatrixUtil.scaleX * getContentWidth() / 2
    val hh: Float = mMatrixUtil.scaleY * getContentHeight() / 2
    val center = getCenter()
    val cx = center!![0]
    val cy = center[1]
    getContentView()?.rotation = mMatrixUtil.getRotationDeg()
    getContentView()?.layout((cx - hw).toInt(), (cy - hh).toInt(), (cx + hw).toInt(), (cy + hh).toInt())
  }

  fun getCenter(): FloatArray? {
    val w = width
    val h = height
    if (w == 0 || h == 0) {
      return null
    }
    val center = FloatArray(2)
    center[0] = width / 2 + mMatrixUtil.translateX
    center[1] = height / 2 + mMatrixUtil.translateY
    return center
  }

  fun reset() {
    mTransform.reset()
    mTransformMode = MODE_DEVICE
    invalidateTransform()
  }

  private fun commit() {
    mListener?.onChange(this)
  }

  fun contentContains(x: Float, y: Float): Boolean {
    fromEditorToContent(x, y)
    val ix = POINT_LIST[0]
    val iy = POINT_LIST[1]
    val isContains: Boolean
    isContains = (abs(ix) <= getContentWidth() / 2
        && abs(iy) <= getContentHeight() / 2)
    return isContains
  }

  private fun converge(): Matrix {
    return mTransform
  }

  protected fun fromEditorToContent(x: Float, y: Float) {
    if (mInverseTransformInvalidated) {
      val together = converge()
      together.invert(mInverseTransform)
      mInverseTransformInvalidated = false
    }
    POINT_LIST[2] = x - width / 2
    POINT_LIST[3] = y - height / 2
    mInverseTransform.mapPoints(POINT_LIST, 0, POINT_LIST, 2, 1)
  }

  protected fun fromContentToEditor(x: Float, y: Float) {
    POINT_LIST[2] = x
    POINT_LIST[3] = y
    mTransform.mapPoints(POINT_LIST, 0, POINT_LIST, 2, 1)
    POINT_LIST[0] += width / 2f
    POINT_LIST[1] += height / 2f
  }

  fun moveContent(dx: Float, dy: Float) {
    mTransform.postTranslate(dx, dy)
    invalidateTransform()
  }

  private fun getScreenPoint(): Point {
    val point = Point()
    val displayMetrics = resources.displayMetrics
    point.x = displayMetrics.widthPixels
    point.y = displayMetrics.heightPixels
    return point
  }

  fun scaleContent(sx: Float, sy: Float) {
    val m = Matrix()
    m.set(mTransform)
    m.preScale(sx, sy)
    mMatrixUtil.decomposeTSR(m)
    val width = (mMatrixUtil.scaleX * getContentWidth())
    val height = (mMatrixUtil.scaleY * getContentHeight())
    val screenPoint = getScreenPoint()
    val thold = screenPoint.x.coerceAtLeast(screenPoint.y).toFloat()
    val minScale = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 15f, resources.displayMetrics)
    if ((width >= thold || height >= thold) && sx > 1) {
    } else if ((width <= minScale || height <= minScale) && sx < 1) {
    } else {
      mTransform.set(m)
    }
    invalidateTransform()
  }

  fun rotateContent(rot: Float) {
    mTransform.postRotate(Math.toDegrees(rot.toDouble()).toFloat())
    mMatrixUtil.decomposeTSR(mTransform)
    invalidateTransform()
  }

  interface OnChangeListener {
    fun onChange(overlay: AbstractPasteView)
  }

  class MatrixUtil {
    var translateX = 0f
    var translateY = 0f
    var scaleX = 0f
    var scaleY = 0f
    var rotation = 0f
    private val data = FloatArray(9)

    fun setRotationDeg(value: Float) {
      rotation = (value * Math.PI / 180).toFloat()
    }

    fun getRotationDeg(): Float {
      return (rotation / Math.PI * 180).toFloat()
    }

    fun decomposeTSR(m: Matrix) {
      m.getValues(data)
      translateX = data[Matrix.MTRANS_X]
      translateY = data[Matrix.MTRANS_Y]
      val sx = data[Matrix.MSCALE_X]
      val sy = data[Matrix.MSCALE_Y]
      val skewx = data[Matrix.MSKEW_X]
      val skewy = data[Matrix.MSKEW_Y]
      scaleX = sqrt(sx * sx + skewx * skewx.toDouble()).toFloat()
      scaleY = sqrt(sy * sy + skewy * skewy.toDouble()).toFloat() * sign(sx * sy - skewx * skewy)
      rotation = atan2(-skewx.toDouble(), sx.toDouble()).toFloat()
    }

    fun composeTSR(m: Matrix) {
      m.setRotate(getRotationDeg())
      m.postScale(scaleX, scaleY)
      m.postTranslate(translateX, translateY)
    }
  }
}