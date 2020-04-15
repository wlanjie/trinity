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

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.ColorFilter
import android.graphics.Paint
import android.graphics.drawable.Drawable
import android.util.AttributeSet
import android.util.TypedValue
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

@Suppress("DEPRECATION")
class WheelView : ScrollView {

  companion object {
    private const val OFF_SET_DEFAULT = 1
    private const val SCROLL_DIRECTION_UP = 0
    private const val SCROLL_DIRECTION_DOWN = 1
  }

  private val items = mutableListOf<DataModel>()
  private var views: LinearLayout? = null
  private var offset = OFF_SET_DEFAULT
  private var displayItemCount = 0
  private var selectedIndex = 1
  private var initialY = 0
  private var scrollerTask: Runnable? = null
  private var newCheck = 50
  private var itemHeight = 0
  private var selectedAreaBorder = intArrayOf()
  private var scrollDirection = -1
  private var paint: Paint? = null
  private var viewWidth = 0
  private var onWheelViewListener: OnWheelViewListener? = null

  interface OnWheelViewListener {
    fun onSelected(selectedIndex: Int, item: String?)
  }

  constructor(context: Context) : super(context) {
    init(context)
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init(context)
  }

  constructor(context: Context, attrs: AttributeSet?, defStyle: Int) : super(context, attrs, defStyle) {
    init(context)
  }

  fun setOffset(offset: Int) {
    this.offset = offset
  }

  fun setItems(list: List<DataModel>?) {
    items.clear()
    items.addAll(list!!)

    // 前面和后面补全
    for (i in 0 until offset) {
      items.add(0, object : DataModel {
        override fun getText(): String {
          return ""
        }
      })
      items.add(object : DataModel {
        override fun getText(): String {
          return ""
        }
      })
    }
    initData()
  }


  private fun init(context: Context) {
    this.isVerticalScrollBarEnabled = false
    views = LinearLayout(context)
    views!!.orientation = LinearLayout.VERTICAL
    this.addView(views)
    scrollerTask = Runnable {
      val newY = scrollY
      if (initialY - newY == 0) {
        val remainder = initialY % itemHeight
        val divided = initialY / itemHeight
        if (remainder == 0) {
          selectedIndex = divided + offset
          onSeletedCallBack()
        } else {
          if (remainder > itemHeight / 2) {
            post {
              smoothScrollTo(0, initialY - remainder + itemHeight)
              selectedIndex = divided + offset + 1
              onSeletedCallBack()
            }
          } else {
            post {
              smoothScrollTo(0, initialY - remainder)
              selectedIndex = divided + offset
              onSeletedCallBack()
            }
          }
        }
      } else {
        initialY = scrollY
        postDelayed(scrollerTask, newCheck.toLong())
      }
    }
  }

  fun startScrollerTask() {
    initialY = scrollY
    postDelayed(scrollerTask, newCheck.toLong())
  }

  private fun initData() {
    displayItemCount = offset * 2 + 1
    for (i in items.indices) {
      views!!.addView(createView(items[i].getText(), i, displayItemCount))
    }
    refreshItemView(0)
  }

  private fun createView(item: String, position: Int, count: Int): TextView {
    val tv = TextView(context)
    tv.layoutParams = LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
    tv.isSingleLine = true
    tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f)
    tv.text = item
    tv.gravity = Gravity.CENTER
    val padding = dip2px(12f)
    tv.setPadding(padding, padding, padding, padding)
    if (0 == itemHeight) {
      itemHeight = getViewMeasuredHeight(tv)
      views!!.layoutParams = LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, itemHeight * displayItemCount)
      val lp = this.layoutParams as LinearLayout.LayoutParams
      this.layoutParams = LinearLayout.LayoutParams(lp.width, itemHeight * displayItemCount)
    }
    return tv
  }

  override fun onScrollChanged(l: Int, t: Int, oldl: Int, oldt: Int) {
    super.onScrollChanged(l, t, oldl, oldt)
    refreshItemView(t)
    scrollDirection = if (t > oldt) {
      SCROLL_DIRECTION_DOWN
    } else {
      SCROLL_DIRECTION_UP
    }
  }

  private fun refreshItemView(y: Int) {
    var position = y / itemHeight + offset
    val remainder = y % itemHeight
    val divided = y / itemHeight
    if (remainder == 0) {
      position = divided + offset
    } else {
      if (remainder > itemHeight / 2) {
        position = divided + offset + 1
      }
    }
    val childSize = views!!.childCount
    for (i in 0 until childSize) {
      val itemView = views!!.getChildAt(i) as TextView ?: return
      if (position == i) {
        itemView.setTextColor(Color.parseColor("#0288ce"))
      } else {
        itemView.setTextColor(Color.parseColor("#bbbbbb"))
      }
    }
  }

  /**
   * 获取选中区域的边界
   */
  private fun obtainSelectedAreaBorder(): IntArray {
    selectedAreaBorder[0] = itemHeight * offset
    selectedAreaBorder[1] = itemHeight * (offset + 1)
    return selectedAreaBorder
  }

  override fun setBackgroundDrawable(background: Drawable?) {
    if (viewWidth == 0) {
      viewWidth = (context as Activity).windowManager.defaultDisplay.width
    }
    if (null == paint) {
      paint = Paint()
      paint?.color = Color.parseColor("#83cde6")
      paint?.strokeWidth = dip2px(1f).toFloat()
    }
    if (selectedAreaBorder.isEmpty()) {
      return
    }
    val drawable = object : Drawable() {
      override fun draw(canvas: Canvas) {
        paint?.let {
          canvas.drawLine(0f, obtainSelectedAreaBorder()[0].toFloat(), viewWidth.toFloat(), obtainSelectedAreaBorder()[0].toFloat(), it)
          canvas.drawLine(0f, obtainSelectedAreaBorder()[1].toFloat(), viewWidth.toFloat(), obtainSelectedAreaBorder()[1].toFloat(), it)
        }
      }

      override fun setAlpha(alpha: Int) {}
      override fun setColorFilter(cf: ColorFilter?) {}

      @SuppressLint("WrongConstant")
      override fun getOpacity(): Int {
        return 0
      }
    }
    super.setBackgroundDrawable(drawable)
  }

  override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
    super.onSizeChanged(w, h, oldw, oldh)
    viewWidth = w
    setBackgroundDrawable(null)
  }

  fun setOnWheelViewListener(l: OnWheelViewListener) {
    onWheelViewListener = l
  }

  private fun onSeletedCallBack() {
    onWheelViewListener?.onSelected(selectedIndex - offset, items[selectedIndex].getText())
  }

  fun setSelection(position: Int) {
    selectedIndex = position + offset
    postDelayed({ scrollTo(0, position * itemHeight) }, 300)
  }

  val seletedItem: String
    get() = items[selectedIndex].getText()

  val seletedIndex: Int
    get() = selectedIndex - offset

  override fun fling(velocityY: Int) {
    super.fling(velocityY / 3)
  }

  override fun onTouchEvent(ev: MotionEvent): Boolean {
    if (ev.action == MotionEvent.ACTION_UP) {
      startScrollerTask()
    }
    return super.onTouchEvent(ev)
  }

  private fun dip2px(dpValue: Float): Int {
    val scale = context!!.resources.displayMetrics.density
    return (dpValue * scale + 0.5f).toInt()
  }

  private fun getViewMeasuredHeight(view: View): Int {
    val width = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED)
    val expandSpec = MeasureSpec.makeMeasureSpec(Int.MAX_VALUE shr 2, MeasureSpec.AT_MOST)
    view.measure(width, expandSpec)
    return view.measuredHeight
  }

  interface DataModel {
    fun getText(): String
  }
}