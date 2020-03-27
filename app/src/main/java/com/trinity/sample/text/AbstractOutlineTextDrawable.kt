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

@file:Suppress("DEPRECATION")

package com.trinity.sample.text

import android.graphics.*
import android.graphics.Paint.Cap
import android.graphics.Paint.Join
import android.graphics.drawable.Drawable
import android.os.Build.VERSION
import android.text.Layout
import android.text.StaticLayout
import android.text.TextPaint
import kotlin.math.max

abstract class AbstractOutlineTextDrawable : Drawable() {
  private var mLayout: StaticLayout? = null
  protected val mTextPaint = TextPaint(1)
  private var mFillColor = 0
  private var mBackgroundColor = 0
  private var mBackgroundBitmap: Bitmap? = null
  private var mStrokeColor = 0
  private var mStrokeWidth = 0f
  protected var mLineSpacingAdd = 0.0f
  protected var mLineSpacingMultiplier = 1.0f
  protected var mIncludeFontPadding = true
  protected var mWidth = 0
  protected var mHeight = 0
  protected var mTextWidth = 0
  protected var mTextHeight = 0
  protected var mFixTextSize = 0
  protected var mTextOffSetX = 0
  protected var mTextOffsetY = 0
  protected var mMaxLines = 0
  protected var mText: CharSequence? = null
  var mAlignment: Layout.Alignment? = null

  fun AbstractOutlineTextDrawable() {
    mAlignment = Layout.Alignment.ALIGN_CENTER
  }

  abstract fun layout(): StaticLayout?

  protected fun invalidateLayout() {
    mLayout = null
    invalidateSelf()
  }

  fun getTextSize(): Float {
    return mTextPaint.textSize
  }

  fun setPaint(paint: TextPaint?) {
    mTextPaint.set(paint)
    invalidateLayout()
  }

  fun setFakeBoldText(value: Boolean) {
    mTextPaint.isFakeBoldText = value
  }

  fun setStrokeJoin(join: Join?) {
    mTextPaint.strokeJoin = join
  }

  fun setStrokeCap(cap: Cap?) {
    mTextPaint.strokeCap = cap
  }

  fun getPaint(): TextPaint? {
    return mTextPaint
  }

  fun setTypeface(face: Typeface?) {
    mTextPaint.typeface = face
    invalidateLayout()
  }

  fun setBackgroundColor(color: Int) {
    mBackgroundColor = color
    invalidateSelf()
  }

  fun setBackgroundBitmap(bitmap: Bitmap?) {
    mBackgroundBitmap = bitmap
    invalidateLayout()
  }

  fun setFillColor(color: Int) {
    mFillColor = color
    invalidateSelf()
  }

  fun setStrokeColor(color: Int) {
    mStrokeColor = color
    invalidateSelf()
  }

  private fun getStrokeWidth(value: Float): Float {
    return max(2.0f, (value - 42.0f) / 15.0f)
  }

  fun getStrokeWidth(): Float {
    return mStrokeWidth
  }

  private fun applyStrokeWidth() {
    val textSize = mTextPaint.textSize
    mStrokeWidth = getStrokeWidth(textSize)
    if (VERSION.SDK_INT == 19) {
      val strokeWidth = if (textSize <= 256.0f) mStrokeWidth else mStrokeWidth / 5.0f
      mTextPaint.strokeWidth = strokeWidth
    } else {
      mTextPaint.strokeWidth = mStrokeWidth
    }
    invalidateSelf()
  }

  fun setAlignment(alignment: Layout.Alignment?) {
    mAlignment = alignment
  }

  fun setText(text: CharSequence?) {
    mText = text
    invalidateLayout()
  }

  fun setLineSpacing(add: Float, mult: Float) {
    mLineSpacingAdd = add
    mLineSpacingMultiplier = mult
    invalidateLayout()
  }

  fun setIncludeFontPadding(value: Boolean) {
    mIncludeFontPadding = value
    invalidateLayout()
  }

  override fun getIntrinsicWidth(): Int {
    return mWidth
  }

  override fun getIntrinsicHeight(): Int {
    return mHeight
  }

  fun setSize(w: Int, h: Int) {
    mWidth = w
    mHeight = h
    invalidateLayout()
  }

  fun setTextSize(w: Int, h: Int) {
    mTextWidth = w
    mTextHeight = h
  }

  fun setFixTextSize(textSize: Int) {
    mFixTextSize = textSize
  }

  fun setTextOffSet(offsetX: Int, offsetY: Int) {
    mTextOffSetX = offsetX
    mTextOffsetY = offsetY
  }

  override fun onBoundsChange(bounds: Rect?) {
    super.onBoundsChange(bounds)
    mLayout = null
  }

  override fun draw(canvas: Canvas) {
    if (mLayout == null) {
      mLayout = layout()
      if (mMaxLines > 0 && mLayout?.lineCount ?: 0 > mMaxLines) {
        val lineStart = mLayout?.getLineStart(mMaxLines) ?: 0
        mText = mText?.subSequence(0, lineStart)
        mLayout = layout()
      }
      applyStrokeWidth()
    }
    canvas.save()
    val bounds = this.bounds
    canvas.drawColor(mBackgroundColor)
    mBackgroundBitmap?.let {
      val src = Rect()
      src.left = 0
      src.top = 0
      src.right = it.width
      src.bottom = it.height
      val dest = Rect()
      dest.left = 0
      dest.right = mWidth
      dest.top = 0
      dest.bottom = mHeight
      canvas.drawBitmap(it, src, dest, null as Paint?)
    }
    canvas.translate(bounds.left.toFloat(), bounds.top.toFloat())
    canvas.scale(bounds.width().toFloat() / mWidth.toFloat(), bounds.height().toFloat() / mHeight.toFloat())
    if (mAlignment == Layout.Alignment.ALIGN_CENTER) {
      canvas.translate(((mWidth - mTextOffSetX - (mLayout?.width ?: 0)) / 2 +
          mTextOffSetX).toFloat(), ((mHeight - mTextOffsetY - (mLayout?.height ?: 0)) / 2 + mTextOffsetY).toFloat())
    } else {
      canvas.translate(mTextOffSetX.toFloat(), mTextOffsetY.toFloat())
    }
    if (mStrokeColor != 0) {
      mLayout?.paint?.color = mStrokeColor
      mLayout?.paint?.style = Paint.Style.FILL_AND_STROKE
      mLayout?.draw(canvas)
    }
    if (mFillColor != 0) {
      mLayout?.paint?.color = mFillColor
      mLayout?.paint?.style = Paint.Style.FILL
      mLayout?.draw(canvas)
    }
    canvas.restore()
  }

  override fun setAlpha(alpha: Int) {}

  override fun setColorFilter(cf: ColorFilter?) {}

  override fun getOpacity(): Int {
    return PixelFormat.TRANSPARENT
  }

  protected fun makeLayout(size: Float): StaticLayout? {
    mTextPaint.textSize = size
    return StaticLayout(mText, mTextPaint, mTextWidth, mAlignment, mLineSpacingMultiplier, mLineSpacingAdd, mIncludeFontPadding)
  }

  protected fun makeLayout(size: Float, width: Int): StaticLayout? {
    mTextPaint.textSize = size
    return StaticLayout(mText, mTextPaint, width, mAlignment, mLineSpacingMultiplier, mLineSpacingAdd, mIncludeFontPadding)
  }

  protected fun makeLayout(text: CharSequence?, size: Float): StaticLayout? {
    mTextPaint.textSize = size
    return StaticLayout(text, mTextPaint, mTextWidth, mAlignment, mLineSpacingMultiplier, mLineSpacingAdd, mIncludeFontPadding)
  }

  fun setMaxLines(maxLines: Int) {
    mMaxLines = maxLines
  }
}