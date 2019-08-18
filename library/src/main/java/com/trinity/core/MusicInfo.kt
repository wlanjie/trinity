package com.trinity.core

/**
 * Create by wlanjie on 2019/4/23-下午3:14
 */
class MusicInfo(val path: String, var timeRange: TimeRange = TimeRange(0, Long.MAX_VALUE)) {

  var looping =false
  var fadeIn = false
  var fadeOut = false
}