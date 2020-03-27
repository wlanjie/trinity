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

import android.util.Log
import com.trinity.sample.entity.EffectCaption
import com.trinity.sample.entity.EffectPaste
import com.trinity.sample.entity.EffectText
import com.trinity.sample.entity.PasteDescriptor
import kotlin.math.min

class PasteConverter {

  companion object {
    private const val DEFAULT_EDIT_SIZE = 640
  }
  private var mDisplayWidth = 0
  private var mDisplayHeight = 0
  private var pasteWidth = 0

  fun setConvertWidthAndHeight(w: Int, h: Int) {
    mDisplayWidth = w
    mDisplayHeight = h
    pasteWidth = min(w, h)
  }

  fun fillDescription(paste: EffectPaste, descriptor: PasteDescriptor) {
    if (pasteWidth != 0 && mDisplayWidth != 0 && mDisplayHeight != 0) {
      val widthRatio = DEFAULT_EDIT_SIZE.toFloat() / pasteWidth.toFloat()
      descriptor.width = paste.width * widthRatio
      descriptor.height = paste.height * widthRatio
      descriptor.x = paste.x / mDisplayWidth.toFloat() * DEFAULT_EDIT_SIZE.toFloat()
      descriptor.y = paste.y / mDisplayHeight.toFloat() * DEFAULT_EDIT_SIZE.toFloat()
      descriptor.mXRatio = paste.xRatio
      descriptor.mYRatio = paste.yRatio
      descriptor.mWidthRatio = paste.widthRatio
      descriptor.mHeightRatio = paste.heightRatio
      descriptor.start = paste.start
      descriptor.end = paste.end
      descriptor.duration = paste.duration
      descriptor.rotation = paste.rotation
      descriptor.kernelFrame = paste.kernelFrame
      descriptor.frameArray = paste.frameArray
      descriptor.timeArray = paste.timeArray
      descriptor.name = paste.name
      descriptor.uri = paste.path
      descriptor.mirror = paste.mirror
      descriptor.id = paste.viewId
      if (paste.getPasteType() == 1) {
        val text: EffectText = paste as EffectText
        descriptor.text = text.text
        descriptor.preTextColor = text.textColor
        descriptor.textColor = text.textColor
        descriptor.preTextStrokeColor = text.dTextStrokeColor
        descriptor.textStrokeColor = text.textStrokeColor
        descriptor.font = text.font
        descriptor.textBmpPath = text.textBmpPath
        descriptor.textWidth = text.textWidth * widthRatio
        descriptor.textHeight = text.textHeight * widthRatio
        descriptor.hasTextLabel = text.hasLabel
        descriptor.textLabelColor = text.textLabelColor
        descriptor.mBackgroundBmpPath = text.backgroundBitmapPath
        descriptor.mTextSize = text.textSize * widthRatio
        descriptor.mTextPaddingX = text.textPaddingX * widthRatio
        descriptor.mTextPaddingY = text.textPaddingY * widthRatio
        descriptor.setTextAlignment(text.mTextAlignment)
        descriptor.mTextMaxLines = text.textMaxLines
      }
      if (paste.getPasteType() == 2) {
        val caption: EffectCaption = paste as EffectCaption
        descriptor.text = caption.text
        descriptor.preTextColor = caption.textColor
        descriptor.textColor = caption.textColor
        descriptor.preTextStrokeColor = caption.dTextStrokeColor
        descriptor.textStrokeColor = caption.textStrokeColor
        descriptor.preTextBegin = caption.preBegin
        descriptor.preTextEnd = caption.preEnd
        descriptor.textRotation = caption.textRotation
        descriptor.font = caption.font
        descriptor.textBmpPath = caption.textBmpPath
        descriptor.gifViewId = caption.gifViewId
        descriptor.hasTextLabel = caption.hasLabel
        descriptor.textLabelColor = caption.textLabelColor
        descriptor.textWidth = caption.textWidth * widthRatio
        descriptor.textHeight = caption.textHeight * widthRatio
        descriptor.textOffsetX = caption.textCenterX * widthRatio
        descriptor.textOffsetY = caption.textCenterY * widthRatio
      }
    } else {
      Log.e("trinity", "Please set valid ConvertWidthAndHeight")
    }
  }

  fun fillPaste(paste: EffectPaste, descriptor: PasteDescriptor, restore: Boolean) {
    val width = descriptor.width
    val height = descriptor.height
    val widthRatio = pasteWidth.toFloat() / DEFAULT_EDIT_SIZE.toFloat()
    paste.width = (width * widthRatio).toInt()
    paste.height = (height * widthRatio).toInt()
    var left = 0.0f
    var top = 0.0f
    if (isValid()) {
      if (!restore) {
        var x = descriptor.x
        var y = descriptor.y
        y *= widthRatio
        x *= widthRatio
        val result: Float
        if (mDisplayWidth > mDisplayHeight) {
          if (DEFAULT_EDIT_SIZE.toFloat() - width != 0.0f) {
            result = (paste.width * 1.0F * mDisplayWidth / mDisplayHeight)
            x += (result - paste.width) / 2.0f * (height - (DEFAULT_EDIT_SIZE / 2)) / ((DEFAULT_EDIT_SIZE.toFloat() - width) / 2.0f)
          }
        } else if (mDisplayHeight > mDisplayWidth && DEFAULT_EDIT_SIZE.toFloat() - height != 0.0f) {
          result = (paste.height * 1.0F * mDisplayHeight / mDisplayWidth)
          y += (result - paste.height) / 2.0f * (descriptor.y - (DEFAULT_EDIT_SIZE / 2)) / ((DEFAULT_EDIT_SIZE.toFloat() - height) / 2.0f)
        }
        left = (mDisplayWidth / 2).toFloat() - x
        top = (mDisplayHeight / 2).toFloat() - y
      }
      paste.x = (descriptor.x * 1.0F / DEFAULT_EDIT_SIZE * mDisplayWidth - left).toInt()
      paste.y = (descriptor.y * 1.0F / DEFAULT_EDIT_SIZE * mDisplayHeight - top).toInt()
    }
    paste.xRatio = descriptor.mXRatio
    paste.yRatio = descriptor.mYRatio
    paste.widthRatio = descriptor.mWidthRatio
    paste.heightRatio = descriptor.mHeightRatio
    paste.start = descriptor.start
    paste.end = descriptor.end
    paste.duration = descriptor.duration
    paste.kernelFrame = descriptor.kernelFrame
    paste.frameArray = descriptor.frameArray
    paste.timeArray = descriptor.timeArray
    paste.name = descriptor.name ?: ""
    paste.rotation = descriptor.rotation
    paste.mirror = descriptor.mirror
  }

  fun fillText(text: EffectText, descriptor: PasteDescriptor) {
    text.text = descriptor.text ?: ""
    text.dTextColor = descriptor.preTextColor
    text.textColor = descriptor.textColor
    text.dTextStrokeColor = descriptor.preTextStrokeColor
    text.textStrokeColor = descriptor.textStrokeColor
    text.hasLabel = descriptor.hasTextLabel
    text.textLabelColor = descriptor.textLabelColor
    text.font = descriptor.font ?: ""
    text.textBmpPath = descriptor.textBmpPath ?: ""
    text.backgroundBitmapPath = descriptor.mBackgroundBmpPath ?: ""
    val width = descriptor.textWidth
    val height = descriptor.textHeight
    text.textWidth = (width / DEFAULT_EDIT_SIZE.toFloat() * pasteWidth.toFloat()).toInt()
    text.textHeight = (height / DEFAULT_EDIT_SIZE.toFloat() * pasteWidth.toFloat()).toInt()
    text.textSize = (descriptor.mTextSize / DEFAULT_EDIT_SIZE * pasteWidth.toFloat()).toInt()
    text.textPaddingX = (descriptor.mTextPaddingX / DEFAULT_EDIT_SIZE * pasteWidth.toFloat()).toInt()
    text.textPaddingY = (descriptor.mTextPaddingY / DEFAULT_EDIT_SIZE * pasteWidth.toFloat()).toInt()
    text.mTextAlignment = descriptor.getTexAlignment()
    text.textMaxLines = descriptor.mTextMaxLines
  }

  fun fillCaption(caption: EffectCaption, descriptor: PasteDescriptor) {
    val width = descriptor.textWidth
    val height = descriptor.textHeight
    val offsetX = descriptor.textOffsetX
    val offsetY = descriptor.textOffsetY
    caption.preBegin = descriptor.preTextBegin
    caption.preEnd = descriptor.preTextEnd
    caption.textRotation = descriptor.textRotation
    caption.textWidth = (width / DEFAULT_EDIT_SIZE.toFloat() * pasteWidth.toFloat()).toInt()
    caption.textHeight = (height / DEFAULT_EDIT_SIZE.toFloat() * pasteWidth.toFloat()).toInt()
    caption.textCenterX = (offsetX / DEFAULT_EDIT_SIZE.toFloat() * pasteWidth.toFloat()).toInt()
    caption.textCenterY = (offsetY / DEFAULT_EDIT_SIZE.toFloat() * pasteWidth.toFloat()).toInt()
  }

  fun isValid(): Boolean {
    return mDisplayWidth > 0 && mDisplayHeight > 0
  }
}