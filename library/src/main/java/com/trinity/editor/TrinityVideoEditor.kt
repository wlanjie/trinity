package com.trinity.editor

import android.view.SurfaceView

interface TrinityVideoEditor {

  /**
   * 设置预览view
   */
  fun setSurfaceView(surfaceView: SurfaceView)

  /**
   * 获取视频时长
   */
  fun getVideoDuration(): Long

  /**
   * 获取视频片段数量
   */
  fun getClipsCount(): Int

  /**
   * 获取视频clip
   * @param index 当前片段的下标
   * @return 下标无效时返回null
   */
  fun getClip(index: Int): MediaClip?

  /**
   * 播入一段clip到队列
   * @param clip 插入的clip对象
   * @return 插入成功返回0
   */
  fun insertClip(clip: MediaClip): Int

  /**
   * 播入一段clip到队列
   * @param index 播入的下标
   * @param clip 插入的clip对象
   * @return 插入成功返回0
   */
  fun insertClip(index: Int, clip: MediaClip): Int

  /**
   * 删除一个片段
   * @param index 删除片段的下标
   */
  fun removeClip(index: Int)

  /**
   * 替换一个片段
   * @param index 需要替换的下标
   * @param clip 需要替换片段的对象
   * @return 替换成功返回0
   */
  fun replaceClip(index: Int, clip: MediaClip): Int

  /**
   * 获取所有片段
   * @return 返回当前所有片段
   */
  fun getVideoClips(): List<MediaClip>

  /**
   * 获取当前片段的时间段
   * @param index 获取的下标
   * @return 返回当前片段的时间段
   */
  fun getClipTimeRange(index: Int): TimeRange

  fun getVideoTime(index: Int, clipTime: Long): Long

  fun getClipTime(index: Int, videoTime: Long): Long

  /**
   * 根据时间获取片段下标
   * @param time 传入时间
   * @return 查找到的坐标, 如果查找不到返回-1
   */
  fun getClipIndex(time: Long): Int

  fun addFilter(lut: ByteArray, startTime: Long, endTime: Long): Int

  /**
   * 开始播放
   * @param repeat 是否循环播放
   * @return 播放成功返回0
   */
  fun play(repeat: Boolean): Int

  /**
   * 暂停播放
   */
  fun pause()

  /**
   * 继续播放
   */
  fun resume()

  /**
   * 停止播放, 释放资源
   */
  fun stop()

  fun destroy()
}