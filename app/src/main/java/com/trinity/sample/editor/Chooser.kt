package com.trinity.sample.editor

import android.content.Context
import android.graphics.Color
import android.util.AttributeSet
import android.view.ViewGroup
import android.widget.FrameLayout
import com.trinity.sample.view.OverlayThumbLineBar
import com.trinity.sample.view.ThumbLineBar

/**
 * Created by wlanjie on 2019-07-24
 */
abstract class Chooser : FrameLayout {

  private var mThumbContainer: FrameLayout ?= null
  protected var mThumbLineBar: OverlayThumbLineBar ?= null

  constructor(context: Context) : super(context) {
    init()
    mThumbContainer = getThumbContainer()
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init()
    mThumbContainer = getThumbContainer()
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    init()
    mThumbContainer = getThumbContainer()
  }

  abstract fun init()

  open fun getThumbContainer(): FrameLayout? {
    return null
  }

  fun getCalculateHeight(): Int {
    var height = measuredHeight
    if (height == 0) {
      measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED)
      height = measuredHeight
    }
    return height
  }

  fun removeOwn() {
    if (parent != null) {
      (parent as ViewGroup).removeView(this)
    }
    onRemove()
  }

  fun onRemove() {
    mThumbContainer?.apply {
      val childAt = getChildAt(0)
      childAt?.let {
        (it as ThumbLineBar).hide()
      }
    }
  }

  fun addThumbView(bar: OverlayThumbLineBar) {
    val parent = bar.parent
    parent?.let {
      (parent as ViewGroup).removeView(bar)
    }
    mThumbContainer?.let {
      it.removeAllViews()
      it.addView(bar)
//      bar.showOverlay(getEditorPage())
      bar.setBackgroundResource(android.R.color.transparent)
      bar.show()
      mThumbLineBar = bar
    }
  }

  fun getEditorPage(): EditorPage {
    return EditorPage.NONE
  }

  abstract fun isPlayerNeedZoom(): Boolean
}