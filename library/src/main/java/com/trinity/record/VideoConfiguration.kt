package com.trinity.record

/**
 * Create by wlanjie on 2019/4/23-下午1:48
 */
class VideoConfiguration {

  // 视频编码的宽必须为16的倍数
  // 软编码可以不受这个控制,但是MediaCodec必须设置16的倍数
  // 否则会出现有绿边等奇怪问题
  var videoWidth = 540
    set(value) {
      field = Math.ceil(value / 16.0).toInt() * 16
    }

  // 说明同宽
  var videoHeight = 960
    set(value) {
      field = Math.ceil(value / 16.0).toInt() * 16
    }

  // 摄像头预览的宽
  var previewWidth = 1280
  // 摄像头预览的高
  var previewHeight = 720

  // 视频编码帧率
  var fps = 30
  // 视频码率, 默认 width * height * 6 3M
  var videoBitRate = videoWidth * videoHeight * 6
  // 视频显示和编码的画幅: 垂直 横向 1:1方形
  var frame = Frame.VERTICAL

  enum class Frame {
    VERTICAL,
    HORIZONTAL,
    SQUARE
  }

  // 音频声道数
  var audioChannels = 1
  // 音频码率 默认 128K
  var audioBitRate = 128 * 1000
}