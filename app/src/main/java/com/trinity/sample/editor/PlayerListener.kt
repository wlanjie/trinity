package com.trinity.sample.editor

/**
 * Created by wlanjie on 2019-07-27
 */
interface PlayerListener {
  fun getCurrentDuration(): Long

  fun getDuration(): Long

  fun updateDuration(duration: Long)
}