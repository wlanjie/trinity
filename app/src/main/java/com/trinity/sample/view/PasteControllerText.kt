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
import android.text.Layout
import com.trinity.sample.entity.EffectText
import com.trinity.sample.text.TextBitmap
import com.trinity.sample.text.TextBitmapGenerator

class PasteControllerText(
    private val effectText: EffectText
) : PasteController, BitmapGenerator {

  private var mPasteView: PasteBaseView ?= null
  private var mBitmapGenerator: TextBitmapGenerator ?= null
  private var mTextBitmap: TextBitmap ?= null

  override fun setPasteView(baseView: PasteBaseView) {
  }

  override fun getText(): String? {
    return effectText.text
  }

  override fun getTextColor(): Int {
    return effectText.textColor
  }

  override fun getConfigTextColor(): Int {
    return effectText.dTextColor
  }

  override fun getConfigTextStrokeColor(): Int {
    return effectText.dTextStrokeColor
  }

  override fun getTextStrokeColor(): Int {
    return effectText.textStrokeColor
  }

  override fun isTextHasStroke(): Boolean {
    return effectText.hasLabel
  }

  override fun getTextBgLabelColor(): Int {
    return effectText.textLabelColor
  }

  override fun getPasteCenterY(): Int {
    return effectText.y
  }

  override fun getPasteCenterX(): Int {
    return effectText.x
  }

  override fun getPasteWidth(): Int {
    return effectText.width
  }

  override fun getPasteHeight(): Int {
    return effectText.height
  }

  override fun getPasteRotation(): Float {
    return effectText.rotation
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
    return effectText.font
  }

  override fun editStart() {
  }

  override fun editCompleted(): Int {
    return 0
  }

  override fun removePaste(): Int {
    return 0
  }

  override fun isPasteExists(): Boolean {
    return true
  }

  override fun setPasteStartTime(start: Long) {
    effectText.end = start + getPasteDuration()
    effectText.start = start
  }

  override fun setPasteDuration(duration: Long) {
    effectText.end = getPasteStartTime() + duration
  }

  override fun getPasteStartTime(): Long {
    return effectText.start
  }

  override fun getPasteDuration(): Long {
    return effectText.end - effectText.start
  }

  override fun getPasteIconPath(): String? {
    return ""
  }

  override fun getPasteType(): Int {
    return 1
  }

  override fun isPasteMirrored(): Boolean {
    return false
  }

  override fun setOnlyApplyUI(var1: Boolean) {
  }

  override fun isOnlyApplyUI(): Boolean {
    return false
  }

  override fun setRevert(var1: Boolean) {
  }

  override fun isRevert(): Boolean {
    return false
  }

  protected fun fillText() {
    mPasteView?.let {
      effectText.textColor = it.getTextColor()
      effectText.textStrokeColor = it.getTextStrokeColor()
      effectText.text = it.getText() ?: ""
      effectText.font = it.getPasteTextFont() ?: ""
      effectText.hasStroke = it.isTextHasStroke()
      effectText.hasLabel = it.isTextHasLabel()
      effectText.textLabelColor = it.getTextBgLabelColor()
      val width = it.getPasteTextWidth()
      if (width != 0) {
        effectText.textWidth = width
      }
      val height = it.getPasteTextHeight()
      if (height != 0) {
        effectText.textHeight = height
      }
      effectText.width = it.getPasteWidth()
      effectText.height = it.getPasteHeight()
      effectText.backgroundBitmap = it.getBackgroundBitmap()
      effectText.textSize = it.getTextFixSize()
      effectText.textPaddingY = it.getTextPaddingY()
      effectText.textPaddingX = it.getTextPaddingX()
      it.getTextAlign()?.apply {
        effectText.mTextAlignment = this
      }
      effectText.textMaxLines = it.getTextMaxLines()
    }
  }

  override fun generateBitmap(width: Int, height: Int): Bitmap? {
    if (mBitmapGenerator == null) {
      mBitmapGenerator = TextBitmapGenerator()
      mTextBitmap = TextBitmap()
    }
    mTextBitmap?.mText = effectText.text
    mTextBitmap?.mFontPath = effectText.font
    mTextBitmap?.mBmpWidth = width
    mTextBitmap?.mBmpHeight = height
    mTextBitmap?.mTextWidth = effectText.textWidth
    mTextBitmap?.mTextHeight = effectText.textHeight
    mTextBitmap?.mTextColor = effectText.textColor
    mTextBitmap?.mTextStrokeColor = effectText.textStrokeColor
    mTextBitmap?.mTextAlignment = Layout.Alignment.ALIGN_CENTER
    mTextBitmap?.mBackgroundColor = effectText.textLabelColor
    mTextBitmap?.mBackgroundBmp = effectText.backgroundBitmap
    mTextBitmap?.mTextSize = effectText.textSize
    mTextBitmap?.mTextPaddingX = effectText.textPaddingX
    mTextBitmap?.mTextPaddingY = effectText.textPaddingY
    mTextBitmap?.mTextAlignment = effectText.mTextAlignment
    mTextBitmap?.mMaxLines = effectText.textMaxLines
    mBitmapGenerator?.updateTextBitmap(mTextBitmap)
    return mBitmapGenerator?.generateBitmap(width, height)
  }


}