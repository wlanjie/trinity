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

package com.trinity.player

import com.trinity.ErrorCode
import java.io.File

class AudioPlayer {

  private var mId = create()

  private external fun create(): Long

  fun start(path: String): Int {
    val file = File(path)
    if (!file.exists()) {
      return ErrorCode.FILE_NOT_EXISTS
    }
    return start(mId, path)
  }

  private external fun start(id: Long, path: String): Int

  fun resume() {
    resume(mId)
  }

  private external fun resume(id: Long)

  fun pause() {
    pause(mId)
  }

  private external fun pause(id: Long)

  fun stop() {
    stop(mId)
  }

  private external fun stop(id: Long)

  fun release() {
    release(mId)
    mId = 0
  }

  private external fun release(id: Long)
}