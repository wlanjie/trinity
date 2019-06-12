package com.trinity.editor

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

class VideoEditor : TrinityVideoEditor, SurfaceHolder.Callback {

  private var mId: Long = 0

  init {
    mId = create()
  }

  override fun setSurfaceView(surfaceView: SurfaceView) {
    surfaceView.holder.addCallback(this)
  }

  override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
    onSurfaceChanged(mId, width, height)
  }

  override fun surfaceDestroyed(holder: SurfaceHolder) {
    onSurfaceDestroyed(mId, holder.surface)
  }

  override fun surfaceCreated(holder: SurfaceHolder) {
    onSurfaceCreated(mId, holder.surface)
  }

  private external fun onSurfaceCreated(handle: Long, surface: Surface)

  private external fun onSurfaceChanged(handle: Long, width: Int, height: Int)

  private external fun onSurfaceDestroyed(handle: Long, surface: Surface)

  /**
   * 创建c++对象
   * @return 返回c++对象地址
   */
  private external fun create(): Long

  /**
   * 获取视频时长
   */
  override fun getVideoDuration(): Long {
    return getVideoDuration(mId)
  }


  private external fun getVideoDuration(id: Long): Long

  /**
   * 获取视频片段数量
   */
  override fun getClipsCount(): Int {
    return getClipsCount(mId)
  }

  private external fun getClipsCount(id: Long): Int

  /**
   * 获取视频clip
   * @param index 当前片段的下标
   * @return 下标无效时返回null
   */
  override fun getClip(index: Int): MediaClip? {
    return getClip(mId, index)
  }

  private external fun getClip(id: Long, index: Int): MediaClip?

  /**
   * 播入一段clip到队列
   * @param clip 插入的clip对象
   * @return 插入成功返回0
   */
  override fun insertClip(clip: MediaClip): Int {
    return insertClip(mId, clip)
  }

  private external fun insertClip(id: Long, clip: MediaClip): Int

  /**
   * 播入一段clip到队列
   * @param index 播入的下标
   * @param clip 插入的clip对象
   * @return 插入成功返回0
   */
  override fun insertClip(index: Int, clip: MediaClip): Int {
    return insertClip(mId, index, clip)
  }

  private external fun insertClip(id: Long, index: Int, clip: MediaClip): Int

  /**
   * 删除一个片段
   * @param index 删除片段的下标
   */
  override fun removeClip(index: Int) {
    removeClip(mId, index)
  }

  private external fun removeClip(id: Long, index: Int)

  /**
   * 替换一个片段
   * @param index 需要替换的下标
   * @param clip 需要替换片段的对象
   * @return 替换成功返回0
   */
  override fun replaceClip(index: Int, clip: MediaClip): Int {
    return replaceClip(mId, index, clip)
  }

  private external fun replaceClip(id: Long, index: Int, clip: MediaClip): Int

  /**
   * 获取所有片段
   * @return 返回当前所有片段
   */
  override fun getVideoClips(): List<MediaClip> {
    return getVideoClips(mId)
  }

  private external fun getVideoClips(id: Long): List<MediaClip>

  /**
   * 获取当前片段的时间段
   * @param index 获取的下标
   * @return 返回当前片段的时间段
   */
  override fun getClipTimeRange(index: Int): TimeRange {
    return getClipTimeRange(mId, index)
  }

  private external fun getClipTimeRange(id: Long, index: Int): TimeRange

  override fun getVideoTime(index: Int, clipTime: Long): Long {
    return getVideoTime(mId, index, clipTime)
  }

  private external fun getVideoTime(id: Long, index: Int, clipTime: Long): Long

  override fun getClipTime(index: Int, videoTime: Long): Long {
    return getClipTime(mId, index, videoTime)
  }

  private external fun getClipTime(id: Long, index: Int, videoTime: Long): Long

  /**
   * 根据时间获取片段下标
   * @param time 传入时间
   * @return 查找到的坐标, 如果查找不到返回-1
   */
  override fun getClipIndex(time: Long): Int {
    return getClipIndex(mId, time)
  }

  private external fun getClipIndex(id: Long, time: Long): Int

  private external fun addAction(): Int

  override fun addFilter(lut: ByteArray): Int {
    return addFilter(mId, lut)
  }

  private external fun addFilter(id: Long, lut: ByteArray): Int

  /**
   * 开始播放
   * @param repeat 是否循环播放
   * @return 播放成功返回0
   */
  override fun play(repeat: Boolean): Int {
    return play(mId, repeat)
  }

  private external fun play(id: Long, repeat: Boolean): Int

  /**
   * 暂停播放
   */
  override fun pause() {
    pause(mId)
  }

  private external fun pause(id: Long)

  /**
   * 继续播放
   */
  override fun resume() {
    resume(mId)
  }

  private external fun resume(id: Long)

  /**
   * 停止播放, 释放资源
   */
  override fun stop() {
    stop(mId)
  }

  private external fun stop(id: Long)

  override fun destroy() {
    release(mId)
    mId = 0
  }

  private external fun release(id: Long)
}