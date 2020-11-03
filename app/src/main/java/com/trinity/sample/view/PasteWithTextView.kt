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
import android.util.AttributeSet
import android.util.Log
import android.view.View
import com.trinity.sample.R

class PasteWithTextView : AbstractPasteView {

  private var isEditCompleted = false
  private var isCouldShowLabel = false
  private var mContentView: View? = null
  private var mTextLabel: View? = null
  private var mContentWidth = 0
  private var mContentHeight = 0

  constructor(context: Context) : super(context)

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs)

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr)

  fun isEditCompleted(): Boolean {
    return isEditCompleted
  }

  override fun setEditCompleted(isEditCompleted: Boolean) {
    val width = getContentWidth()
    val height = getContentHeight()
    this.isEditCompleted = isEditCompleted
    if (width == 0 || height == 0) {
      return
    }
    if (isEditCompleted) {
      requestLayout()
    }
    Log.d("EDIT", "EditCompleted : " + isEditCompleted + "mContentWidth : " + mContentWidth
        + " mContentHeight : " + mContentHeight)
  }

  override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
    validateTransform()
    mMatrixUtil.decomposeTSR(mTransform)
    if (isEditCompleted) {
      val width = (mMatrixUtil.scaleX * mContentWidth).toInt()
      val height = (mMatrixUtil.scaleY * mContentHeight).toInt()
      Log.d("EDIT", "Measure width : " + width + "scaleX : " + mMatrixUtil.scaleX +
          "mContentWidth : " + mContentWidth
          + " mContentHeight : " + mContentHeight)
      val w = MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY)
      val h = MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY)
      super.onMeasure(widthMeasureSpec, heightMeasureSpec)
      measureChildren(w, h)
    } else {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec)
      measureChildren(widthMeasureSpec, heightMeasureSpec)
    }
  }

  override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
    super.onLayout(changed, l, t, r, b)
    if (mTextLabel == null) {
      isCouldShowLabel = false
      return
    }
    mMatrixUtil.decomposeTSR(mTransform)
    if (mMatrixUtil.getRotationDeg() == 0f) {
      isCouldShowLabel = true
      val center = getCenter()
      val cy = center?.get(1) ?: 0F
      val hh = mMatrixUtil.scaleY * getContentHeight() / 2
      mTextLabel?.layout(0, (cy - hh).toInt(), width, (cy + hh).toInt())
    } else {
      mTextLabel?.layout(0, 0, 0, 0)
      isCouldShowLabel = false
    }
  }

  override fun setContentWidth(width: Int) {
    mContentWidth = width
  }

  override fun setContentHeight(height: Int) {
    mContentHeight = height
  }

  override fun isCouldShowLabel(): Boolean {
    return isCouldShowLabel
  }

  override fun onFinishInflate() {
    super.onFinishInflate()
    mContentView = findViewById(android.R.id.content)
    mTextLabel = findViewById(R.id.overlay_text_label)
  }

  override fun setShowTextLabel(isShow: Boolean) {
    mTextLabel?.visibility = if (isShow) View.VISIBLE else View.GONE
  }

  override fun getTextLabel(): View? {
    return mTextLabel
  }

  override fun getContentWidth(): Int {
    return if (isEditCompleted) {
      mContentWidth
    } else mContentView?.measuredWidth ?: 0
  }

  override fun getContentHeight(): Int {
    return if (isEditCompleted) {
      mContentHeight
    } else mContentView?.measuredHeight ?: 0
  }

  override fun getContentView(): View? {
    return mContentView
  }

}