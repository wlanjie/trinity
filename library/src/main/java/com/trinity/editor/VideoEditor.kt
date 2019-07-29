package com.trinity.editor

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.trinity.ErrorCode
import java.io.File

class VideoEditor : TrinityVideoEditor, SurfaceHolder.Callback {

  companion object {
    const val NO_ACTION = -1
  }

  private var mId: Long = 0
  private var mFilterActionId = NO_ACTION

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

  override fun getCurrentPosition(): Long {
    return getCurrentPosition(mId)
  }

  private external fun getCurrentPosition(handle: Long): Long

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

  /**
   * 添加滤镜
   * @param lut 色阶图buffer
   * @param startTime 从哪里开始加
   * @param endTime 到哪里结束
   * @return 滤镜对应的id, 删除或者更新滤镜时需要用到
   */
  override fun addFilter(lut: ByteArray, startTime: Long, endTime: Long): Int {
    mFilterActionId = addFilter(mId, lut, startTime, endTime, mFilterActionId)
    return mFilterActionId
  }

  private external fun addFilter(id: Long, lut: ByteArray, startTime: Long, endTime: Long, actionId: Int): Int

  /**
   * 添加背景音乐
   * @param path 音乐路径
   * @param startTime 从哪里开始
   * @param endTime 到哪里结束
   * @return
   */
  override fun addMusic(path: String, startTime: Long, endTime: Long): Int {
    val file = File(path)
    if (!file.exists() || file.length() == 0L) {
      // 文件不存在
      return ErrorCode.FILE_NOT_FOUND
    }
    return addMusic(mId, path, startTime, endTime)
  }

  private external fun addMusic(id: Long, path: String, startTime: Long, endTime: Long): Int

  override fun addAction(type: EffectType, startTime: Long, endTime: Long) {
    addAction(mId, type.ordinal, startTime, endTime)
  }

  private external fun addAction(handle: Long, type: Int, startTime: Long, endTime: Long)

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

  override fun export(
    exportConfig: String,
    path: String,
    width: Int,
    height: Int,
    frameRate: Int,
    videoBitRate: Int,
    sampleRate: Int,
    channelCount: Int,
    audioBitRate: Int
  ): Int {
    return export(mId, exportConfig, path, width, height, frameRate, videoBitRate, sampleRate, channelCount, audioBitRate)
  }

  private external fun export(id: Long, path: String, width: Int, height: Int, frameRate: Int, videoBitRate: Int, sampleRate: Int, channelCount: Int, audioBitRate: Int): Int

  private external fun export(id: Long, exportConfig: String, path: String, width: Int, height: Int, frameRate: Int, videoBitRate: Int, sampleRate: Int, channelCount: Int, audioBitRate: Int): Int

  override fun destroy() {
    release(mId)
    mId = 0
  }

  private external fun release(id: Long)

  override fun testTranscode() {
    testTranscode(mId)
  }

  private external fun testTranscode(id: Long)
}