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

import android.graphics.Color
import android.text.TextUtils
import java.io.Serializable

class DynamicImage : Serializable {
  companion object {
    const val TYPE_TEXT = 0
    const val TYPE_FACE = 1
    const val TYPE_NORMAL = 2

    fun getTextOnlyConfig(): DynamicImage {
      val image = DynamicImage()
      image.pid = 1L
      image.fid = 1L
      image.du = 1F
      image.type = 0
      image.x = 320F
      image.y = 320F
      image.w = 470F
      image.h = 160F
      image.a = 0F
      image.fx = 0F
      image.fy = 0F
      image.fw = 0F
      image.fh = 0F
      image.n = "text_sample"
      image.c = 8F
      image.pText = "请输入文字"
      image.tFont = "SentyTEA"
      image.tMinSize = 21F
      image.tSize = 48F
      image.tLeft = 235F
      image.tTop = 80F
      image.tWidth = 460F
      image.tHeight = 120F
      image.strokeR = 51
      image.strokeG = 51
      image.strokeB = 51
      image.strokeWidth = 4
      image.pExtend = 1
      image.extendSection = 0
      image.tR = 255
      image.tG = 255
      image.tB = 255
      image.tBegin = 0F
      image.tEnd = 1F
      return image
    }
  }

  var path = ""
  var pid = 0L
  var fid = 0L
  var du = 0F
  var type = 0
  var x = 0F
  var y = 0F
  var w = 0F
  var h = 0F
  var a = 0F
  var fx = 0F
  var fy = 0F
  var fw = 0F
  var fh = 0F
  var n = ""
  var c = 0F
  var pText = ""
  var tFont = ""
  var fontId = 0L
  var tSize = 0F
  var tMaxSize = 0F
  var tMinSize = 0F
  var tAngle = 0F
  var tLeft = 0F
  var tTop = 0F
  var tWidth = 0F
  var tHeight = 0F
  var tR = 0
  var tG = 0
  var tB = 0
  var tBegin = 0F
  var tEnd = 0F
  var hasTextLabel = false
  var textLabelColor = 0
  var variableLab = false
  var variableLabType = ""
  var variableLabText = ""
  var variableLabLeft = 0F
  var variableLabTop = 0F
  var variableLabWidth = 0F
  var variableLabHeight = 0F
  var strokeR = 0
  var strokeG = 0
  var strokeB = 0
  var strokeWidth = 0
  var pExtend = 0
  var extendSection = 0
  var frameArray = mutableListOf<Frame>()
  var timeArray = mutableListOf<FrameTime>()
  var kernelFrame = -1

  fun validate(): Boolean {
    return if (TextUtils.equals("text_sample", n)) {
      true
    } else if (frameArray.isNotEmpty()) {
      timeArray.isNotEmpty()
    } else {
      false
    }
  }

  private fun kernelFrame(): Int {
    if (kernelFrame == -1) {
      return kernelFrame
    } else {
      if (extendSection == 1 && timeArray.size > 1) {
        val beginTime = timeArray[1].beginTime.toFloat()
        frameArray.forEach {
          if (beginTime == it.time) {
            return it.pic
          }
        }
      }
    }
    return if (frameArray.isNotEmpty()) frameArray[frameArray.lastIndex - 1].pic else 0
  }

  fun getTextColor(): Int {
    return Color.argb(255, tR, tG, tB)
  }

  fun getTextStrokeColor(): Int {
    return Color.argb(255, strokeR, strokeG, strokeB)
  }

  fun fillPasteDescription(descriptor: PasteDescriptor) {
    descriptor.width = w
    descriptor.height = h
    descriptor.x = x
    descriptor.y = y
    descriptor.kernelFrame = kernelFrame()
    descriptor.name = n
    descriptor.duration = (du * 1000.0f * 1000.0f).toLong()
    descriptor.start = 0L
    descriptor.end = descriptor.duration
    descriptor.preTextBegin = (tBegin * 1000.0f * 1000.0f).toLong()
    descriptor.preTextEnd = (tEnd * 1000.0f * 1000.0f).toLong()
    descriptor.preTextColor = getTextColor()
    descriptor.preTextStrokeColor = getTextStrokeColor()
    descriptor.rotation = 0.0f
    descriptor.textRotation = Math.toRadians(tAngle.toDouble()).toFloat()
    descriptor.text = pText
    descriptor.textOffsetX = tLeft
    descriptor.textOffsetY = tTop
    descriptor.textWidth = tWidth
    descriptor.textHeight = tHeight
    descriptor.frameArray = frameArray
    descriptor.timeArray = timeArray
    descriptor.textColor = descriptor.preTextColor
    descriptor.textStrokeColor = descriptor.preTextStrokeColor
    descriptor.textLabelColor = textLabelColor
    descriptor.hasTextLabel = hasTextLabel
  }
}