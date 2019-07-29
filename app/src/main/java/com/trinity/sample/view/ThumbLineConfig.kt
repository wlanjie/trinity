package com.trinity.sample.view

import android.graphics.Point

/**
 * Created by wlanjie on 2019-07-27
 */
class ThumbLineConfig private constructor() {

  var thumbnailFetcher: ThumbnailFetcher? = null
    private set
  var thumbnailCount = 10
    private set
  var thumbnailPoint: Point? = null
    private set
  var screenWidth: Int = 0
    private set

  class Builder {
    private val mConfig = ThumbLineConfig()

    fun thumbnailFetcher(thumbnailFetcher: ThumbnailFetcher): Builder {
      mConfig.thumbnailFetcher = thumbnailFetcher
      return this
    }

    fun thumbPoint(point: Point): Builder {
      mConfig.thumbnailPoint = point
      return this
    }

    fun screenWidth(screenWidth: Int): Builder {
      mConfig.screenWidth = screenWidth
      return this
    }

    fun thumbnailCount(thumbnailCount: Int): Builder {
      mConfig.thumbnailCount = thumbnailCount
      return this
    }

    fun build(): ThumbLineConfig {
      return mConfig
    }

  }
}
