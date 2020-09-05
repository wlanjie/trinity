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

package com.trinity.sample.entity

import android.graphics.Bitmap
import android.text.Layout
import java.security.MessageDigest

open class EffectText(path: String) : EffectPaste(path) {
  var textAlignment = Layout.Alignment.ALIGN_NORMAL
  var text = ""
  var textBmpPath = ""
  var textWidth = 0
  var textHeight = 0
  var textColor = 0
  var dTextColor = 0
  var textStrokeColor = 0
  var dTextStrokeColor = 0
  var font = ""
  var hasStroke = false
  var hasLabel = false
  var textLabelColor = 0
  var backgroundBitmap: Bitmap ?= null
  var backgroundBitmapPath = ""
  var textSize = 0
  var textPaddingX = 0
  var textPaddingY = 0
  var mTextAlignment = Layout.Alignment.ALIGN_NORMAL
  var needSaveBitmap = true
  var textMaxLines = 0

  fun generateTextFinger(): String {
    val string = buildString {
      append(textAlignment).
      append(viewId).
      append(text).
      append(textWidth).
      append(textHeight).
      append(width).
      append(height).
      append(textColor).
      append(textStrokeColor).
      append(font).
      append(hasLabel).
      append(hasStroke).
      append(textLabelColor)
    }
    val md5 = MessageDigest.getInstance("MD5")
    md5.update(string.toByteArray())
    return getHash(md5)
  }

  private fun getHash(md5: MessageDigest): String {
    val buffer = md5.digest()
    val length = buffer.size
    return buildString {
      for (i in 0..length) {
        val b = buffer[i].toInt()
        append(Integer.toHexString(b shl 4 and 15)).
        append(Integer.toHexString(b and 15))
      }
    }
  }

  override fun getPasteType(): Int {
    return 1
  }

  override fun copy(base: EffectBase) {
    super.copy(base)
    val text = base as EffectText
    text.textAlignment = textAlignment
    text.text = this.text
    text.textBmpPath = textBmpPath
    text.textWidth = textWidth
    text.textHeight = textHeight
    text.textColor = textColor
    text.dTextColor = dTextColor
    text.textStrokeColor = textStrokeColor
    text.font = font
    text.hasStroke = hasStroke
    text.hasLabel = hasLabel
    text.textLabelColor = textLabelColor
    text.backgroundBitmap = backgroundBitmap
    text.backgroundBitmapPath = backgroundBitmapPath
    text.textSize = textSize
    text.textPaddingX = textPaddingX
    text.textPaddingY = textPaddingY
    text.mTextAlignment = mTextAlignment
    text.needSaveBitmap = needSaveBitmap
    text.textMaxLines = textMaxLines
  }
}