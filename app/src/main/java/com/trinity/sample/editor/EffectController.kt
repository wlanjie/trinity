package com.trinity.sample.editor

import android.content.Context
import android.graphics.Color
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.trinity.editor.TrinityVideoEditor
import com.trinity.sample.R
import com.trinity.sample.entity.Effect
import com.trinity.sample.view.OverlayThumbLineBar
import com.trinity.sample.view.ThumbLineOverlay
import java.util.*

class EffectController(private val context: Context, private val videoEditor: TrinityVideoEditor) {

  companion object {
    private const val MESSAGE_ADD_OVERLAY = 0
    private const val MESSAGE_UPDATE_PROGRESS = 1
    private const val MESSAGE_REMOVE_OVERLAY = 2
    private const val MESSAGE_STOP_TO_UPDATE_OVERLAY = 3
    private const val MESSAGE_CLEAR_ALL_ANIMATION_FILTER = 4
    private const val MESSAGE_RESTORE_ANIMATION_FILTER = 5
  }

  private var mThumbLineBar: OverlayThumbLineBar ?= null
  private var mOverlayColor = 0
  private var mOverlayHandler = TimelineBarOverlayHandler(Looper.getMainLooper())
  private var mCurrOverlayView: OverlayView? = null
  private val mAddedFilter = Stack<Effect>()
  private var mAddedFilterTemp: Stack<Effect> ?= null
  private val mAddedOverlay = Stack<ThumbLineOverlay>()
  private var mAddedOverlayTemp: Stack<ThumbLineOverlay>? = null
  private var mCurrOverlay: ThumbLineOverlay? = null
  private var mInvert = false
  private var mLastStartTime: Long = 0

  fun onEventAnimationFilterLongClick(info: Effect) {
    mLastStartTime = info.startTime.toLong()
    selectOverlayColor(info)
    mOverlayHandler.sendEmptyMessage(MESSAGE_ADD_OVERLAY)
    if (mAddedFilterTemp == null) {
      mAddedFilterTemp = Stack()
      mAddedFilterTemp?.addAll(mAddedFilter)
    }
    mAddedFilterTemp?.push(info)
  }

  fun onEventAnimationFilterClickUp(info: Effect) {
    mAddedFilterTemp?.let {
      if (it.isNotEmpty()) {
        val lastFilter = it[it.size - 1]
        mOverlayHandler.sendEmptyMessage(MESSAGE_STOP_TO_UPDATE_OVERLAY)
      }
    }
  }

  fun onEventAnimationFilterDelete(info: Effect) {
    if (mAddedFilterTemp == null) {
      mAddedFilterTemp = Stack<Effect>()
      mAddedFilterTemp?.addAll(mAddedFilter)
    }
    mAddedFilterTemp?.let {
     if (it.isNotEmpty())  {
       val lastFilter = it.pop()
       mOverlayHandler.sendEmptyMessage(MESSAGE_REMOVE_OVERLAY)
     }
    }
  }

  fun onEventClearAnimationFilter() {
    if (mAddedFilterTemp != null) {
      mAddedFilterTemp?.clear()
      mAddedFilterTemp = null
      mOverlayHandler.sendEmptyMessage(MESSAGE_CLEAR_ALL_ANIMATION_FILTER)
    }
  }

  fun onEventConfirmAnimationFilter() {
    mAddedOverlay.clear()
    mAddedFilter.clear()
    mAddedOverlayTemp?.let {
      mAddedOverlay.addAll(it)
    }
    mAddedFilterTemp?.let {
      mAddedFilter.addAll(it)
    }
    mAddedOverlayTemp?.clear()
    mAddedFilterTemp?.clear()
    mAddedOverlayTemp = null
    mAddedFilterTemp = null
  }

  private fun addOverlay() {
    mCurrOverlayView = OverlayView(context, mInvert)
    val duration = videoEditor.getCurrentPosition() - mLastStartTime
    mCurrOverlayView?.let {
      mCurrOverlay = mThumbLineBar?.addOverlay(mLastStartTime, duration, it, 0, mInvert, EditorPage.FILTER_EFFECT)
      mCurrOverlay?.updateMiddleViewColor(mOverlayColor)
      mOverlayHandler.obtainMessage(MESSAGE_UPDATE_PROGRESS).sendToTarget()
      if (mAddedOverlayTemp == null) {
        mAddedOverlayTemp = Stack()
        mAddedOverlayTemp?.addAll(mAddedOverlay)
      }
      mAddedOverlayTemp?.push(mCurrOverlay)
    }
  }

  private fun updateProgress() {
    val duration = videoEditor.getCurrentPosition() - mLastStartTime
    mCurrOverlay?.updateDuration(duration)
    mOverlayHandler.obtainMessage(MESSAGE_UPDATE_PROGRESS).sendToTarget()
  }

  private fun removeOverlay() {
    if (mAddedOverlayTemp == null) {
      mAddedOverlayTemp = Stack()
      mAddedOverlayTemp?.addAll(mAddedOverlay)
    }
    mAddedOverlayTemp?.let {
      val overlay = it.pop()
      mThumbLineBar?.removeOverlay(overlay)
      mThumbLineBar?.seekTo(overlay.startTime, false)
      mCurrOverlay = null
      mCurrOverlayView = null
      if (!it.empty()) {
        mCurrOverlay = it.peek()
        mCurrOverlayView = mCurrOverlay?.thumbLineOverlayView as OverlayView
      }
    }
  }

  inner class TimelineBarOverlayHandler(looper: Looper) : Handler(looper) {

    override fun handleMessage(msg: Message) {
      super.handleMessage(msg)
      when (msg.what) {
        MESSAGE_ADD_OVERLAY -> {
          addOverlay()
        }

        MESSAGE_UPDATE_PROGRESS -> {
          updateProgress()
        }

        MESSAGE_STOP_TO_UPDATE_OVERLAY -> {
          removeMessages(MESSAGE_UPDATE_PROGRESS)
        }

        MESSAGE_REMOVE_OVERLAY -> {
          removeOverlay()
        }

        MESSAGE_CLEAR_ALL_ANIMATION_FILTER -> {

        }

        MESSAGE_RESTORE_ANIMATION_FILTER -> {

        }
      }
    }
  }

  fun setThumbLineBar(thumbLineBar: OverlayThumbLineBar) {
    mThumbLineBar = thumbLineBar
  }

  private fun selectOverlayColor(ef: Effect) {
//    var colorRes = R.color.effect_color1
//    val path = ef.path
//    when {
//      path.contains("幻影") -> colorRes = R.color.effect_color1
//      path.contains("重影") -> colorRes = R.color.effect_color2
//      path.contains("抖动") -> colorRes = R.color.effect_color3
//      path.contains("朦胧") -> colorRes = R.color.effect_color4
//      path.contains("科幻") -> colorRes = R.color.effect_color5
//    }
    mOverlayColor = Color.parseColor(ef.color)
  }

  class OverlayView(context: Context, isInvert: Boolean) : ThumbLineOverlay.ThumbLineOverlayView {

    private val mMiddleView: View
    private val mHeadView: View
    private val mTailView: View

    private val mRootView = if (isInvert) {
      LayoutInflater.from(context).inflate(R.layout.timeline_invert_overlay, null)
    } else {
      LayoutInflater.from(context).inflate(R.layout.timeline_overlay, null)
    }

    init {
      mMiddleView = mRootView.findViewById(R.id.middle_view)
      mHeadView = mRootView.findViewById(R.id.head_view)
      mTailView = mRootView.findViewById(R.id.tail_view)
      mHeadView.visibility = View.INVISIBLE
      val lpHead = mHeadView.layoutParams
      val lpTail = mTailView.layoutParams
      lpHead.width = 1
      lpHead.height = 1
      lpTail.width = 1
      lpTail.height = 1
      mTailView.visibility = View.INVISIBLE
      mHeadView.layoutParams = lpHead
      mTailView.layoutParams = lpTail
    }

    override fun getContainer(): ViewGroup {
      return mRootView as ViewGroup
    }

    override fun getHeadView(): View {
      return mRootView.findViewById(R.id.head_view)
    }

    override fun getTailView(): View {
      return mRootView.findViewById(R.id.tail_view)
    }

    override fun getMiddleView(): View {
      return mRootView.findViewById(R.id.middle_view)
    }

  }
}