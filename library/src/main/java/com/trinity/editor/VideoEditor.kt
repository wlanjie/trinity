/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package com.trinity.editor

import android.content.Context
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.tencent.mars.xlog.Log
import com.trinity.ErrorCode
import com.trinity.listener.OnRenderListener
import java.io.File

class VideoEditor(
  context: Context
) : TrinityVideoEditor, SurfaceHolder.Callback {

  companion object {
    const val NO_ACTION = -1
  }

  private var mId: Long = 0
  private var mFilterActionId = NO_ACTION
  // texture回调
  // 可以做特效处理 textureId是TEXTURE_2D类型
  private var mOnRenderListener: OnRenderListener ?= null

  init {
    val path = context.externalCacheDir?.absolutePath + "/resource.json"
    mId = create(path)
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
  private external fun create(resourcePath: String): Long

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
  override fun addFilter(configPath: String): Int {
    mFilterActionId = addFilter(mId, configPath)
    return mFilterActionId
  }

  private external fun addFilter(id: Long, config: String): Int

  override fun updateFilter(configPath: String, startTime: Int, endTime: Int, actionId: Int) {
    updateFilter(mId, configPath, startTime, endTime, actionId)
  }

  private external fun updateFilter(id: Long, config: String, startTime: Int, endTime: Int, actionId: Int)

  override fun deleteFilter(actionId: Int) {
    deleteFilter(mId, actionId)
  }

  private external fun deleteFilter(id: Long, actionId: Int)

  /**
   * 添加背景音乐
   * @param path 音乐路径
   * @param startTime 从哪里开始
   * @param endTime 到哪里结束
   * @return
   */
  override fun addMusic(config: String): Int {
//    val file = File(path)
//    if (!file.exists() || file.length() == 0L) {
//      // 文件不存在
//      return ErrorCode.FILE_NOT_FOUND
//    }
    return addMusic(mId, config)
  }

  private external fun addMusic(id: Long, config: String): Int

  override fun updateMusic(config: String, actionId: Int) {
    if (mId <= 0) {
      return
    }
    updateMusic(mId, config, actionId)
  }

  private external fun updateMusic(id: Long, config: String, actionId: Int)

  override fun deleteMusic(actionId: Int) {
    if (mId <= 0) {
      return
    }
    deleteMusic(mId, actionId)
  }

  private external fun deleteMusic(id: Long, actionId: Int)

  override fun addAction(configPath: String): Int {
    if (mId <= 0) {
      return -1
    }
    return addAction(mId, configPath)
  }

  private external fun addAction(handle: Long, config: String): Int

  override fun updateAction(startTime: Int, endTime: Int, actionId: Int) {
    if (mId <= 0) {
      return
    }
    updateAction(mId, startTime, endTime, actionId)
  }

  private external fun updateAction(handle: Long, startTime: Int, endTime: Int, actionId: Int)

  override fun deleteAction(actionId: Int) {
    if (mId <= 0) {
      return
    }
    deleteAction(mId, actionId)
  }

  private external fun deleteAction(handle: Long, actionId: Int)

  override fun setOnRenderListener(l: OnRenderListener) {
    mOnRenderListener = l
  }

  /**
   * texture回调
   * 可以做其它特效处理, textureId为普通 TEXTURE_2D 类型
   * @param textureId Int
   * @param width Int
   * @param height Int
   */
  @Suppress("unused")
  private fun onDrawFrame(textureId: Int, width: Int, height: Int): Int {
    return mOnRenderListener?.onDrawFrame(textureId, width, height, null) ?: textureId
  }

  override fun seek(time: Int) {
    if (mId <= 0) {
      return
    }
    seek(mId, time)
  }

  private external fun seek(id: Long, time: Int)

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

  @Suppress("unused")
  private fun onPlayStatusChanged(status: Int) {

  }

  @Suppress("unused")
  private fun onPlayError(error: Int) {

  }
}