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

class EffectCaption(path: String) : EffectText(path) {
  var gifViewId = 0
  var textBegin: Long = 0
  var textEnd: Long = 0
  var preBegin: Long = 0
  var preEnd: Long = 0
  var textCenterX = 0
  var textCenterY = 0
  var textRotation = 0f

  override fun getPasteType(): Int {
    return 2
  }

  override fun copy(base: EffectBase) {
    super.copy(base)
    if (base is EffectCaption) {
      base.gifViewId = gifViewId
      base.textBegin = textBegin
      base.textEnd = textEnd
      base.preBegin = preBegin
      base.preEnd = preEnd
      base.textCenterX = textCenterX
      base.textCenterY = textCenterY
      base.textRotation = textRotation
    }
  }
}