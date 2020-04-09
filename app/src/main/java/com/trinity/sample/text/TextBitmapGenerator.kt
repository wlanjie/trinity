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

package com.trinity.sample.text

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint.Join
import android.graphics.Typeface
import android.graphics.drawable.Drawable
import android.text.TextUtils
import android.util.Log
import com.trinity.sample.view.BitmapGenerator
import java.io.File
import java.util.*

class TextBitmapGenerator : BitmapGenerator {

  private val mCanvas = Canvas()
  private val mTextBox: TextBoxDrawable = TextBoxDrawable()
  private var mTextBitmap: TextBitmap? = null

  fun TextBitmapGenerator() {}

  fun updateTextBitmap(text: TextBitmap?) {
    mTextBitmap = text
  }

  fun generateTextBitmap(text: TextBitmap): Bitmap? {
    mTextBitmap = text
    configureLayout(mTextBox, text)
    mTextBox.layout()
    return getImageAligned(mTextBox, text.mBmpWidth, text.mBmpHeight)
  }

  private fun configureLayout(drawable: AbstractOutlineTextDrawable, text: TextBitmap) {
    var typefaceConfig: TypefaceConfig
    if (TextUtils.isEmpty(text.mFontPath)) {
      typefaceConfig = TypefaceConfig()
    } else {
      try {
        val fontPath = File(text.mFontPath)
        val parentFile = fontPath.parentFile
        val ttf = parentFile?.list { _, name -> name.toLowerCase(Locale.ROOT).endsWith(".ttf") }
        typefaceConfig = if (ttf?.size ?: 0 > 0) {
          val ttfPath = ttf?.get(0) ?: return
          val ttfFile = File(parentFile, ttfPath)
          if (!ttfFile.exists()) {
            Log.e("AliYunLog", "Font file[" + ttfFile.absolutePath + "] not exist!")
            TypefaceConfig()
          } else {
            TypefaceConfig(Typeface.createFromFile(ttfFile))
          }
        } else {
          TypefaceConfig()
        }
      } catch (e: Exception) {
        Log.e("AliYunLog", "Load font error!", e)
        typefaceConfig = TypefaceConfig()
      }
    }
    drawable.setTextOffSet(text.mTextPaddingX, text.mTextPaddingY)
    drawable.setFixTextSize(text.mTextSize)
    drawable.setTextSize(text.mTextWidth, text.mTextHeight)
    drawable.setSize(text.mBmpWidth, text.mBmpHeight)
    drawable.setText(text.mText)
    drawable.setFillColor(text.mTextColor)
    drawable.setAlignment(text.mTextAlignment)
    drawable.setStrokeColor(text.mTextStrokeColor)
    drawable.setStrokeJoin(Join.ROUND)
    drawable.setBackgroundColor(text.mBackgroundColor)
    drawable.setBackgroundBitmap(text.mBackgroundBmp)
    drawable.setTypeface(typefaceConfig.typeface)
    drawable.setFakeBoldText(typefaceConfig.fakeBold)
    drawable.setMaxLines(text.mMaxLines)
  }

  private fun getImageAligned(drawable: Drawable, w: Int, h: Int): Bitmap? {
    return rasterize(drawable, w, h)
  }

  private fun rasterize(drawable: Drawable, w: Int, h: Int): Bitmap? {
    val bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
    mCanvas.setBitmap(bitmap)
    drawable.setBounds(0, 0, w, h)
    val count = mCanvas.save()
    drawable.draw(mCanvas)
    mCanvas.restoreToCount(count)
    return bitmap
  }

  fun align2(x: Int, align: Int): Int {
    return x + align - 1 and (align - 1).inv()
  }

  override fun generateBitmap(width: Int, height: Int): Bitmap? {
    val textBitmap = mTextBitmap ?: return null
    mTextBitmap?.mBmpWidth = width
    mTextBitmap?.mBmpHeight = height
    configureLayout(mTextBox, textBitmap)
    mTextBox.layout()
    return getImageAligned(mTextBox, width, height)
  }
}