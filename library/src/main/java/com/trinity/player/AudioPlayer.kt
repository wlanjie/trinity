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