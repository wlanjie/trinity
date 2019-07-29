package com.trinity.sample.view

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.editor.EditorPage
import java.util.*

class OverlayThumbLineBar (
    context: Context,
    attrs: AttributeSet? = null
) : ThumbLineBar(context, attrs) {

  private val mOverlayList = ArrayList<ThumbLineOverlay>()

  /**
   * 添加overlay
   *
   * @param overlayView overlayView
   * @param tailView    tailView
   * @param overlay     overlay
   */
  fun addOverlayView(
      overlayView: View?,
      tailView: ThumbLineOverlayHandleView?,
      overlay: ThumbLineOverlay?,
      isIvert: Boolean) {
    addView(overlayView)
    val view = tailView?.view

    overlayView?.post {
      val layoutParams = view?.layoutParams as ViewGroup.MarginLayoutParams
      if (isIvert) {
        layoutParams.rightMargin = calculateTailViewInvertPosition(tailView)
      } else {
        layoutParams.leftMargin = calculateTailViewPosition(tailView)
      }
      view.requestLayout()
      overlay?.setVisibility(true)
    }
  }

  fun addOverlay(
      time: Long,
      duration: Long,
      view: ThumbLineOverlay.ThumbLineOverlayView,
      minDuration: Long,
      isInvert: Boolean,
      uiEditorPage: EditorPage,
      listener: ThumbLineOverlay.OnSelectedDurationChangeListener? = null
  ): ThumbLineOverlay {
    var startTime = time
    if (startTime < 0) {
      startTime = 0
    }
    view.container.tag = uiEditorPage

    if (mLinePlayer != null) {
      mDuration = mLinePlayer.getDuration()
    }
    val overlay = ThumbLineOverlay(this, startTime, duration, view, mDuration, minDuration, isInvert, listener)
    overlay.uiEditorPage = uiEditorPage
    mOverlayList.add(overlay)
    return overlay
  }

  /**
   * 实现和recyclerView的同步滑动
   * @param dx x的位移量
   * @param dy y的位移量
   */
  override fun onRecyclerViewScroll(dx: Int, dy: Int) {
    super.onRecyclerViewScroll(dx, dy)
    val length = mOverlayList.size
    for (i in 0 until length) {
      mOverlayList[i].requestLayout()
    }

  }

  /**
   * 实现和recyclerView的同步滑动
   */
  override fun onRecyclerViewScrollStateChanged(newState: Int) {
    super.onRecyclerViewScrollStateChanged(newState)

    when (newState) {
      RecyclerView.SCROLL_STATE_IDLE -> for (overlay in mOverlayList) {
        overlay.requestLayout()
      }
      else -> {
      }
    }
  }

  /**
   * 计算overlay尾部view左面的margin值
   * @param tailView view
   * @return int 单位sp
   */
  internal fun calculateTailViewPosition(tailView: ThumbLineOverlayHandleView?): Int {
    return if (tailView?.view != null) {
      (mThumbLineConfig.screenWidth / 2 - tailView.view.measuredWidth + duration2Distance(tailView.duration) - mCurrScroll).toInt()
    } else {
      0
    }
  }

  /**
   * 计算在倒放时overlay尾部view右边的margin值
   * @param tailView view
   * @return 单位sp
   */
  internal fun calculateTailViewInvertPosition(tailView: ThumbLineOverlayHandleView?): Int {
    return if (tailView?.view != null) {
      (mThumbLineConfig.screenWidth / 2 - tailView.view.measuredWidth - duration2Distance(tailView.duration) + mCurrScroll).toInt()
    } else {
      0
    }
  }

  /**
   * 时间转为尺寸
   *
   * @param duration 时长
   * @return 尺寸 pixel
   */
  internal fun duration2Distance(duration: Long): Int {
    val lenth = timelineBarViewWidth * duration.toFloat() * 1.0f / mDuration
    return Math.round(lenth)
  }

  /**
   * 尺寸转为时间
   *
   * @param distance 尺寸 pixel
   * @return long duration
   */
  internal fun distance2Duration(distance: Float): Long {
    val lenth = mDuration * distance / timelineBarViewWidth
    return Math.round(lenth).toLong()
  }

  /**
   * 清除指定
   */
  fun removeOverlay(overlay: ThumbLineOverlay?) {
    if (overlay != null) {
      Log.d(TAG, "remove TimelineBar Overlay : " + overlay.uiEditorPage)
      removeView(overlay.overlayView)
      mOverlayList.remove(overlay)
    }
  }

  fun removeOverlayByPages(vararg uiEditorPages: EditorPage) {
    if (uiEditorPages.isEmpty()) {
      return
    }
    val uiEditorPageList = Arrays.asList(*uiEditorPages)
    var i = 0
    while (i < childCount) {
      val childAt = getChildAt(i)
      if (childAt.tag is ThumbLineOverlay) {
        val thumbLineOverlay = childAt.tag as ThumbLineOverlay
        val uiEditorPage = thumbLineOverlay.uiEditorPage
        if (uiEditorPageList.contains(uiEditorPage)) {
          removeOverlay(thumbLineOverlay)
          i--//这里用i--，有时间换成迭代器来删除
        }
      }
      i++
    }
  }

  /**
   * 显示指定的overlay
   * @param uiEditorPage UIEditorPage
   */
  fun showOverlay(uiEditorPage: EditorPage?) {
    if (uiEditorPage == null) {
      return
    }
    val isCaption = uiEditorPage == EditorPage.FONT || uiEditorPage == EditorPage.CAPTION
    for (overlay in mOverlayList) {
      if (uiEditorPage === overlay.uiEditorPage) {
        overlay.overlayView?.visibility = View.VISIBLE
      } else if (isCaption && (overlay.uiEditorPage == EditorPage.CAPTION || overlay.uiEditorPage == EditorPage.FONT)) {
        overlay.overlayView?.visibility = View.VISIBLE
      } else {
        overlay.overlayView?.visibility = View.INVISIBLE
      }
    }
  }

  fun clearOverlay() {
    for (overlay in mOverlayList) {
      removeView(overlay.overlayView)
    }
    mOverlayList.clear()
  }

  companion object {
    private val TAG = OverlayThumbLineBar::class.java.name
  }

}
