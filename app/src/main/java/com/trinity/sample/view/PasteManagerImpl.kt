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

import com.trinity.sample.entity.DynamicImage
import com.trinity.sample.entity.EffectText
import com.trinity.sample.entity.PasteDescriptor

class PasteManagerImpl : PasteManager {

  private val mConverter = PasteConverter()

  override fun addPaste(path: String) {
    TODO("Not yet implemented")
  }

  override fun addPasteWithStartTime(path: String, startTime: Long, duration: Long): PasteController {
    TODO("Not yet implemented")
  }

  override fun addSubtitle(text: String, font: String): PasteController {
    return addSubtitleWithStartTime(text, font, 0, 0)
  }

  override fun addSubtitleWithStartTime(text: String, font: String, startTime: Long, duration: Long): PasteController {
    val effectText = EffectText(font)
    val image = DynamicImage.getTextOnlyConfig()
    val descriptor = PasteDescriptor()
    image.fillPasteDescription(descriptor)
    if (startTime != 0L) {
      descriptor.start = startTime
    }
    mConverter.fillPaste(effectText, descriptor, false)
    mConverter.fillText(effectText, descriptor)
    effectText.text = text
    effectText.font = font
    return PasteControllerText(effectText)
  }

  override fun setDisplaySize(width: Int, height: Int) {
    TODO("Not yet implemented")
  }

  override fun setOnPasteRestoreListener(l: (List<PasteController>) -> Unit) {
    TODO("Not yet implemented")
  }

}