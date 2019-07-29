package com.trinity.sample.view

import android.graphics.Bitmap

/**
 * Created by wlanjie on 2019-07-27
 */
interface ThumbnailFetcher {

  fun addVideoSource(path: String)

  fun requestThumbnailImage(times: LongArray, l: OnThumbnailCompletionListener)

  fun getTotalDuration(): Long

  fun release()

  interface OnThumbnailCompletionListener {
    fun onThumbnail(bitmap: Bitmap, time: Long)
    fun onError(errorCode: Int)
  }
}
