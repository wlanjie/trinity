/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
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
 *
 */

package com.trinity.editor

import java.io.File
import java.io.FileInputStream

class MediaClip {

  enum class Type {
    GIF,
    IMAGE,
    MP4,
    NONE
  }

  private var path = ""
  private var timeRange: TimeRange ?= null
  private var type = -1

  constructor(path: String) {
    this.path = path
  }

  constructor(path: String, timeRange: TimeRange) {
    this.path = path
    this.timeRange = timeRange
  }

  private fun checkFileType(path: String) {
    val file = File(path)
    if (!file.exists()) {
      return
    }
    val stream = FileInputStream(file)
    val header = ByteArray(12)
    val result = stream.read(header, 0, header.size)
    if (result == -1) {
      return
    }

    if (header[0] == 'G'.toByte() && header[1] == 'I'.toByte() && header[2] == 'F'.toByte()) {
      type = 0
    } else if ((header[1] == 'P'.toByte() && header[2] == 'N'.toByte() && header[3] == 'G'.toByte())
      || (header[6] == 'J'.toByte() && header[7] == 'F'.toByte() && header[8] == 'I'.toByte() && header[9] == 'F'.toByte())
      || (header[6] == 'E'.toByte() && header[7] == 'x'.toByte() && header[8] == 'i'.toByte() && header[9] == 'f'.toByte())) {
      type = 1
    }
    stream.close()
  }
}