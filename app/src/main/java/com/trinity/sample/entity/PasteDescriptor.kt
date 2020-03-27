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

import android.text.Layout

class PasteDescriptor {
  var uri: String? = null
  var name: String? = null
  var mWidthRatio = 0f
  var mHeightRatio = 0f
  var mXRatio = 0f
  var mYRatio = 0f
  var width = 0f
  var height = 0f
  var y = 0f
  var x = 0f
  var rotation = 0f
  var text: String? = null
  var textBmpPath: String? = null
  var textColor = 0
  var preTextColor = 0
  var textLabelColor = 0
  var start: Long = 0
  var end: Long = 0
  var duration: Long = 0
  var font: String? = null
  var faceId = 0
  var type = 0
  var mirror = false
  var hasTextLabel = false
  var maxTextSize = 0
  var textHeight = 0f
  var textStrokeColor = 0
  var preTextStrokeColor = 0
  var textWidth = 0f
  var textOffsetX = 0f
  var textOffsetY = 0f
  var preTextBegin: Long = 0
  var preTextEnd: Long = 0
  var textRotation = 0f
  var kernelFrame = 0
  var frameArray = mutableListOf<Frame>()
  var timeArray = mutableListOf<FrameTime>()
  var isTrack = false
  var id = 0
  var gifViewId = 0
  var mBackgroundBmpPath: String? = null
  var mTextSize = 0.0f
  var mTextPaddingX = 0.0f
  var mTextPaddingY = 0.0f
  var mTextMaxLines = 0
  var mTextAlignment = 0

  fun setTextAlignment(alignment: Layout.Alignment) {
    mTextAlignment = alignment.ordinal
  }

  fun getTexAlignment(): Layout.Alignment {
    return if (mTextAlignment in 0..2) Layout.Alignment.values()[mTextAlignment] else Layout.Alignment.ALIGN_CENTER
  }
}