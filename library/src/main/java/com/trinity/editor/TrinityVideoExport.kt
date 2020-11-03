package com.trinity.editor

import com.trinity.listener.OnExportListener

interface VideoExport {

  /**
   * 开始导出
   * @param path 录制的视频保存的地址
   * @param width 录制视频的宽, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
   * @param height 录制视频的高, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
   * @param videoBitRate 视频输出的码率, 如果设置的是2000, 则为2M, 最终输出的和设置的可能有差别
   * @param frameRate 视频输出的帧率
   * @param sampleRate 音频的采样率
   * @param channelCount 音频的声道数
   * @param audioBitRate 音频的码率
   * @param l 导出回调 包含成功 失败 和进度回调
   * @return Int ErrorCode.SUCCESS 为成功,其它为失败
   */
  fun export( info: VideoExportInfo, l: OnExportListener): Int

  /**
   * 取消合成
   */
  fun cancel()

  /**
   * 释放资源
   */
  fun release()
}