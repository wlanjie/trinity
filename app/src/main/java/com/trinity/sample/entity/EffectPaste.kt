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

open class EffectPaste(path: String) : EffectBase() {
  companion object {
    const val PASTE_TYPE_GIF = 0
    const val PASTE_TYPE_TEXT = 1
    const val PASTE_TYPE_CAPTION = 2
  }

  private var isPasteReady = false
  var name = ""
  var width = 0
  var height = 0
  var start = 0L
  var end = 0L
  var x = 0
  var y = 0
  var rotation = 0F
  var duration = 0L
  var kernelFrame = 0
  var frameArray = mutableListOf<Frame>()
  var timeArray = mutableListOf<FrameTime>()
  var xRatio = 0F
  var yRatio = 0F
  var widthRatio = 0F
  var heightRatio = 0F
  var mirror = false
  var isTrack = true

  open fun getPasteType(): Int {
    return 0
  }
}