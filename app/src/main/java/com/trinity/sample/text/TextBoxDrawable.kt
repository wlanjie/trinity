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

import android.text.StaticLayout

class TextBoxDrawable : AbstractOutlineTextDrawable() {

  override fun layout(): StaticLayout? {
    var textHeight = 0.0f
    var height = mTextHeight.toFloat()
    var result = (textHeight + height) / 2.0f
    var staticLayout: StaticLayout?
    if (mFixTextSize > 0) {
      staticLayout = makeLayout(mFixTextSize.toFloat())
    } else {
      while (true) {
        staticLayout = makeLayout(result)
        if (result == textHeight) {
          break
        }
        if (result == height) {
          result = textHeight
        } else {
          val staticLayoutHeight = staticLayout?.height ?: 0
          if (staticLayoutHeight < mTextHeight) {
            textHeight = result
            result = (result + height) / 2.0f
          } else {
            if (staticLayoutHeight <= mTextHeight) {
              break
            }
            height = result
            result = (textHeight + result) / 2.0f
          }
        }
      }
    }
    return staticLayout
  }
}