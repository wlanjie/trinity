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

import java.io.File
import java.io.FileReader

open class EffectBase {

  companion object {
    private var BASE_EFFECT_ID = 8192
  }

  var resId = 0
  var path = ""
  var viewId = 0
  var animationType = 0

  init {
    synchronized(this) {
      BASE_EFFECT_ID++
      resId = BASE_EFFECT_ID
    }
  }
  
  fun readString(path: String): String {
    val buffer = StringBuffer()
    val file = File(path)
    if (file.exists() && file.length() > 0) {
      val reader = FileReader(file)
      var length: Int
      while (true) {
        length = reader.read()
        if (length == -1) {
          break
        }
        buffer.append(length.toChar())
        reader.close()
      }
    }
    return buffer.toString()
  }

  open fun copy(base: EffectBase) {
    base.path = path
    base.resId = resId
    base.animationType = animationType
    base.viewId = viewId
  }
}