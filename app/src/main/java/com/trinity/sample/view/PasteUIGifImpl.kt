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

import android.view.ViewGroup
import com.trinity.sample.R
import com.trinity.sample.editor.EditorPage

open class PasteUIGifImpl(
    private val pasteView: AbstractPasteView,
    private val pasteController: PasteController,
    private val thumbLineBar: OverlayThumbLineBar
) : PasteUISimpleImpl(pasteView, pasteController, thumbLineBar) {

  init {
    mEditorPage = EditorPage.OVERLAY
    mText = pasteView.getContentView()?.findViewById(R.id.overlay_content_text)
    val width = pasteController.getPasteWidth()
    val height = pasteController.getPasteHeight()
    pasteView.setContentWidth(width)
    pasteView.setContentHeight(height)
    mirrorPaste(pasteController.isPasteMirrored())
    pasteView.rotateContent(pasteController.getPasteRotation())
  }

  override fun moveToCenter() {
    mMoveDelay = true
    pasteView.post {
      val cx = pasteController.getPasteCenterX()
      val cy = pasteController.getPasteCenterY()
      val pcx = (pasteView.parent as ViewGroup).width
      val pcy = (pasteView.parent as ViewGroup).height
      pasteView.moveContent(cx - pcx / 2F, cy - pcy / 2F)
    }
  }

  override fun getPasteWidth(): Int {
    val scale = pasteView.getScale()
    scale?.let {
      val scaleX = it[0]
      val width = pasteView.getContentWidth()
      return (width * scaleX).toInt()
    }
    return 0
  }

  override fun getPasteHeight(): Int {
    val scale = pasteView.getScale()
    scale?.let {
      val scaleY = it[1]
      val height = pasteView.getContentHeight()
      return (height * scaleY).toInt()
    }
    return 0
  }

  override fun getPasteCenterX(): Int {
    if (mMoveDelay) {
      return 0
    }
    val center = pasteView.getCenter()
    center?.let {
      return it[0].toInt()
    }
    return 0
  }

  override fun getPasteCenterY(): Int {
    if (mMoveDelay) {
      return 0
    }
    val center = pasteView.getCenter()
    center?.let {
      return it[1].toInt()
    }
    return 0
  }

  override fun mirrorPaste(mirror: Boolean) {
    super.mirrorPaste(mirror)

  }

  override fun playPasteEffect() {

  }

  override fun stopPasteEffect() {
  }

  override fun getPasteRotation(): Float {
    return pasteView.rotation
  }

  override fun editTimeCompleted() {
    super.editTimeCompleted()
  }
}