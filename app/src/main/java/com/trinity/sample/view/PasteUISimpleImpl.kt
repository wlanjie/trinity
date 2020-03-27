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

import android.graphics.Bitmap
import android.graphics.Color
import android.graphics.PointF
import android.text.Layout
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.FragmentActivity
import com.trinity.sample.R
import com.trinity.sample.editor.EditorPage
import com.trinity.sample.text.AutoResizingTextView
import com.trinity.sample.text.TextDialog
import kotlin.math.atan2

abstract class PasteUISimpleImpl(
    private val pasteBaseView: AbstractPasteView,
    private val pasteController: PasteController,
    private val thumbLineBar: OverlayThumbLineBar
) : PasteBaseView {

  private var mLastX = 0F
  private var mLastY = 0F
  protected var mText: AutoResizingTextView ?= null
  private var mThumbLineOverlay: ThumbLineOverlay ?= null
  private var mIsDelete = false
  private var mIsEditStarted = false
  protected var mMoveDelay = false
  protected var mEditorPage = EditorPage.FONT

  init {
    pasteBaseView.tag = this
    pasteController.setPasteView(this)
    val transform = pasteBaseView.findViewById<View>(R.id.overlay_transform)
    transform?.let {
      val rotationScaleBinding = View.OnTouchListener { v, event ->
        when (event.actionMasked) {
          MotionEvent.ACTION_DOWN -> {
            mLastX = v.left + event.x
            mLastY = v.top + event.y
          }
          MotionEvent.ACTION_MOVE -> {
            update(v.left + event.x, v.top + event.y)
          }
        }
        true
      }
      it.setOnTouchListener(rotationScaleBinding)
    }
    val cancel = pasteBaseView.findViewById<View>(R.id.overlay_cancel)
    cancel?.setOnClickListener {
      removePaste()
    }
    val mirror = pasteBaseView.findViewById<View>(R.id.overlay_mirror)
    mirror?.setOnClickListener {
      val isMirror = pasteBaseView.isMirror()
      mirrorPaste(!isMirror)
    }
    editorTimeStart()
  }

  open fun mirrorPaste(mirror: Boolean) {
    pasteBaseView.setMirror(mirror)
  }

  abstract fun moveToCenter()

  fun getController(): PasteController {
    return pasteController
  }

  fun removePaste() {
    mIsDelete = true
    pasteController.removePaste()
    val parent = pasteBaseView.parent
    parent?.let {
      (parent as ViewGroup).removeView(pasteBaseView)
    }
    thumbLineBar.removeOverlay(mThumbLineOverlay)
  }

  private fun update(x: Float, y: Float) {
    val content = pasteBaseView.getContentView()
    content?.let {
      val x0 = it.left + it.width / 2F
      val y0 = it.top + it.height / 2F
      val dx = x - x0
      val dy = y - y0
      val dx0 = mLastX - x0
      val dy0 = mLastY - y0
      val scale = PointF.length(dx, dy) / PointF.length(dx0, dy0)
      val rot = (atan2(y - y0, x - x0) - atan2(mLastY - y0, mLastX - x0))
      if (scale.isNaN() || scale.isInfinite() || rot.isInfinite() || rot.isNaN()) {
        return@let
      }
      mLastX = x
      mLastY = y
      pasteBaseView.scaleContent(scale, scale)
      pasteBaseView.rotateContent(rot)
    }
  }

  fun getEditorPage(): EditorPage {
    return mEditorPage
  }

  override fun getTextMaxLines(): Int {
    return 0
  }

  override fun getTextAlign(): Layout.Alignment? {
    return null
  }

  override fun getTextPaddingX(): Int {
    return 0
  }

  override fun getTextPaddingY(): Int {
    return 0
  }

  override fun getTextFixSize(): Int {
    return 0
  }

  override fun getBackgroundBitmap(): Bitmap? {
    return null
  }

  override fun getText(): String? {
    return null
  }

  override fun getTextColor(): Int {
    return 0
  }

  override fun getTextStrokeColor(): Int {
    return 0
  }

  override fun isTextHasStroke(): Boolean {
    return false
  }

  override fun isTextHasLabel(): Boolean {
    return false
  }

  override fun getTextBgLabelColor(): Int {
    return 0
  }

  override fun getPasteTextOffsetX(): Int {
    return 0
  }

  override fun getPasteTextOffsetY(): Int {
    return 0
  }

  override fun getPasteTextWidth(): Int {
    return 0
  }

  override fun getPasteTextHeight(): Int {
    return 0
  }

  override fun getPasteTextRotation(): Float {
    return 0F
  }

  override fun getPasteTextFont(): String? {
    return null
  }

  override fun getPasteWidth(): Int {
    return 0
  }

  override fun getPasteHeight(): Int {
    return 0
  }

  override fun getPasteCenterY(): Int {
    return 0
  }

  override fun getPasteCenterX(): Int {
    return 0
  }

  override fun getPasteRotation(): Float {
    return 0F
  }

  override fun transToImage(): Bitmap? {
    return null
  }

  override fun getPasteView(): View? {
    return pasteBaseView
  }

  override fun getTextView(): View? {
    return mText
  }

  override fun isPasteMirrored(): Boolean {
    return pasteBaseView.isMirror()
  }

  fun isPasteRemoved(): Boolean {
    return mIsDelete
  }

  fun isPasteExists(): Boolean {
    return pasteController.isPasteExists()
  }

  fun editorTimeStart() {
    if (mIsEditStarted) {
      return
    }
    mIsEditStarted = true
    pasteBaseView.visibility = View.VISIBLE
    pasteBaseView.bringToFront()
    playPasteEffect()
    pasteController.editStart()
    mThumbLineOverlay?.switchState(ThumbLineOverlay.STATE_ACTIVE)
  }

  abstract fun playPasteEffect()

  abstract fun stopPasteEffect()

  open fun editTimeCompleted() {
    if (!mIsEditStarted || !isPasteExists() || !isPasteRemoved()) {
      return
    }
    if (!pasteController.isRevert() && !pasteController.isOnlyApplyUI() &&
        pasteBaseView.width == 0 && pasteBaseView.height == 0) {
      pasteController.removePaste()
      return
    }
    mIsEditStarted = false
    pasteBaseView.visibility = View.GONE
    stopPasteEffect()
    pasteController.editCompleted()
    mMoveDelay = false
    mThumbLineOverlay?.switchState(ThumbLineOverlay.STATE_FIX)
  }

  private fun hideOverlayView() {
    mThumbLineOverlay?.overlayView?.visibility = View.INVISIBLE
  }

  fun isEditCompleted(): Boolean {
    return !isPasteRemoved() && !mIsEditStarted
  }

  fun contentContains(x: Float, y: Float): Boolean {
    return pasteBaseView.contentContains(x, y)
  }

  fun moveContent(dx: Float, dy: Float) {
    pasteBaseView.moveContent(dx, dy)
  }

  fun isVisibleInTime(time: Long): Boolean {
    val start = pasteController.getPasteStartTime()
    val duration = pasteController.getPasteDuration()
    return time >= start && time <= start + duration
  }

  fun showTextEdit(isInvert: Boolean) {
    mText?.let {
      it.setEditCompleted(true)
      pasteBaseView.setEditCompleted(true)
      val info = TextDialog.EditTextInfo()
      info.dTextColor = pasteController.getConfigTextColor()
      info.dTextStrokeColor = pasteController.getConfigTextStrokeColor()
      // TODO
      info.isTextOnly = true
      info.text = it.text.toString()
      info.textColor = it.currentTextColor
      info.textStrokeColor = it.getTextStrokeColor()
      info.font = it.getFontPath() ?: ""
//      info.animationSelect
      info.layoutWidth = (pasteBaseView.parent as ViewGroup).width
      if (info.isTextOnly) {
        info.textWidth = getPasteWidth()
        info.textHeight = getPasteHeight()
      } else {
        info.textWidth = getPasteTextWidth()
        info.textHeight = getPasteTextHeight()
      }
      (pasteBaseView.parent as ViewGroup).isEnabled = false
      pasteBaseView.visibility = View.GONE
      val textDialog = TextDialog.newInstance(info, isInvert)
      textDialog.setOnStateChangeListener(object: TextDialog.OnStateChangeListener {
        override fun onTextEditCompleted(result: TextDialog.EditTextInfo?) {
          val vg = pasteBaseView.parent as? ViewGroup
          vg?.apply {
            isEnabled = true
            if (result?.text?.isEmpty() == true && mEditorPage == EditorPage.FONT) {
              removePaste()
              return@apply
            }
            it.text = result?.text
            it.setCurrentColor(result?.textColor ?: Color.WHITE)
            it.setTextStrokeColor(result?.textStrokeColor ?: Color.WHITE)
            if (result?.isTextOnly == true) {
              pasteBaseView.setContentWidth(result.textWidth)
              pasteBaseView.setContentHeight(result.textHeight)
            }
            it.setFontPath(result?.font ?: "")
            // anim
            it.setEditCompleted(true)
            pasteBaseView.setEditCompleted(true)
            if (mIsEditStarted) {
              pasteBaseView.visibility = View.VISIBLE
            }
          }
        }
      })
      textDialog.show((pasteBaseView.context as FragmentActivity).supportFragmentManager, "textedit")
    }
  }

  fun showTimeEdit() {
    if (!isPasteExists()) {
      return
    }
    if (mThumbLineOverlay == null) {
      val rootView = LayoutInflater.from(pasteBaseView.context)
        .inflate(R.layout.layout_timeline_overlay, null)
      val overlayView = object: ThumbLineOverlay.ThumbLineOverlayView {

        override fun getContainer(): ViewGroup {
          return rootView as ViewGroup
        }

        override fun getHeadView(): View {
          return rootView.findViewById(R.id.head_view)
        }

        override fun getTailView(): View {
          return rootView.findViewById(R.id.tail_view)
        }

        override fun getMiddleView(): View {
          return rootView.findViewById(R.id.middle_view)
        }

      }
      mThumbLineOverlay = thumbLineBar.addOverlay(pasteController.getPasteStartTime(),
        pasteController.getPasteDuration() / 1000,
        overlayView, 1000, false, mEditorPage,
        object: ThumbLineOverlay.OnSelectedDurationChangeListener {
          override fun onDurationChange(startTime: Long, endTime: Long, duration: Long) {
            pasteController.setPasteStartTime(startTime)
            pasteController.setPasteDuration(duration)
            // set play time
          }
        })
    }
    mThumbLineOverlay?.switchState(ThumbLineOverlay.STATE_ACTIVE)
  }
}