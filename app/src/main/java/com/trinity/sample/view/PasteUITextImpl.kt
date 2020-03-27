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
import android.text.Layout
import com.trinity.sample.editor.EditorPage
import com.trinity.sample.text.AutoResizingTextView

class PasteUITextImpl(
    private val pasteView: AbstractPasteView,
    private val pasteController: PasteController,
    private val thumbLineBar: OverlayThumbLineBar,
    val completed: Boolean
) : PasteUIGifImpl(pasteView, pasteController, thumbLineBar) {

  init {
    mEditorPage = EditorPage.FONT
    if (mText == null) {
      mText = pasteView.getContentView() as AutoResizingTextView
    }
    mText?.text = pasteController.getText()
    mText?.setTextOnly(true)
    mText?.setFontPath(pasteController.getPasteTextFont() ?: "")
    mText?.setTextAngle(pasteController.getPasteRotation())
    mText?.setTextStrokeColor(pasteController.getTextStrokeColor())
    mText?.setCurrentColor(pasteController.getTextColor())
    if (completed) {
      mText?.setTextWidth(pasteController.getPasteWidth())
      mText?.setTextHeight(pasteController.getPasteHeight())
      mText?.setEditCompleted(true)
      pasteView.setEditCompleted(true)
    } else {
      mText?.setEditCompleted(false)
      pasteView.setEditCompleted(false)
    }
  }

  override fun mirrorPaste(mirror: Boolean) {
  }

  override fun getText(): String? {
    return mText?.text?.toString() ?: ""
  }

  override fun getTextColor(): Int {
    return mText?.getTextColor() ?: Color.WHITE
  }

  override fun getPasteTextFont(): String? {
    return mText?.getFontPath() ?: ""
  }

  override fun getTextStrokeColor(): Int {
    return mText?.getTextStrokeColor() ?: Color.WHITE
  }

  override fun isTextHasStroke(): Boolean {
    return getTextStrokeColor() == 0
  }

  override fun isTextHasLabel(): Boolean {
    return pasteView.getTextLabel() != null
  }

  override fun getTextBgLabelColor(): Int {
    return Color.parseColor("#00000000")
  }

  override fun getBackgroundBitmap(): Bitmap? {
    return null
  }

  override fun transToImage(): Bitmap? {
    return mText?.layoutToBitmap()
  }

  override fun getPasteTextWidth(): Int {
    return mText?.getTextWidth() ?: 0
  }

  override fun getPasteTextHeight(): Int {
    return mText?.getTextHeight() ?: 0
  }

  override fun getTextFixSize(): Int {
    return 0
  }

  override fun getTextPaddingX(): Int {
    return 0
  }

  override fun getTextPaddingY(): Int {
    return 0
  }

  override fun getTextAlign(): Layout.Alignment? {
    val layout = mText?.layout
    return layout?.alignment ?: Layout.Alignment.ALIGN_CENTER
  }
}