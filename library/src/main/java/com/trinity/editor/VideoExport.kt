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
import com.trinity.encoder.MediaCodecSurfaceEncoder
import com.trinity.listener.OnExportListener
import com.trinity.util.Trinity

/**
 * Created by wlanjie on 2019-07-30
 */
class TrinityVideoExport(private val context: Context) : VideoExport {

  private var mHandle = create()
  // 硬编码对象
  private var mSurfaceEncoder = MediaCodecSurfaceEncoder()
  // Surface 对象
  private var mSurface: Surface ?= null
  private var mListener: OnExportListener ?= null

  override fun export(info: VideoExportInfo, l: OnExportListener): Int {
    mListener = l
    val resourcePath = context.externalCacheDir?.absolutePath + "/resource.json"
    return export(mHandle, resourcePath, info.path,
      info.width, info.height, info.frameRate, info.videoBitRate,
      info.sampleRate, info.channelCount, info.audioBitRate,
      info.mediaCodecDecode, info.mediaCodecEncode)
  }

  override fun cancel() {
    if (mHandle <= 0) {
      return
    }
    cancel(mHandle)
  }

  override fun release() {
    if (mHandle <= 0) {
      return
    }
    release(mHandle)
    mHandle = 0
  }

  private external fun create(): Long

  private external fun export(handle: Long, resourcePath: String, path: String,
                              width: Int, height: Int, frameRate: Int,
                              videoBitRate: Int, sampleRate: Int, channelCount: Int,
                              audioBitRate: Int, mediaCodecDecode: Boolean, mediaCodecEncode: Boolean): Int

  private external fun cancel(handle: Long)

  private external fun release(handle: Long)

  /**
   * 导出进度,由c++回调回来,不要做改变
   * @param progress 导出的进度, 进度比为0.0-1.0
   */
  @Suppress("unused")
  private fun onExportProgress(progress: Float) {
    mListener?.let {
      Trinity.callback {
        it.onExportProgress(progress)
      }
    }
  }

  /**
   * 导出失败了之后,由c++回调回来
   * @param error 错误码
   */
  @Suppress("unused")
  private fun onExportFailed(error: Int) {
    mListener?.let {
      Trinity.callback {
        it.onExportFailed(error)
      }
    }
  }

  /**
   * 导出完成之后,由c++回调回来
   */
  @Suppress("unused")
  private fun onExportComplete() {
    mListener?.let {
      Trinity.callback {
        it.onExportComplete()
      }
    }
  }

  /**
   * 导出取消了之后,由c++回调回来
   */
  @Suppress("unused")
  private fun onExportCanceled() {
    mListener?.let {
      Trinity.callback {
        it.onExportCanceled()
      }
    }
  }


  /**
   * 由c++回调回来
   * 创建硬编码
   * @param width 录制视频的宽
   * @param height 录制视频的高
   * @param videoBitRate 录制视频的码率
   * @param frameRate 录制视频的帧率
   */
  @Suppress("unused")
  private fun createMediaCodecSurfaceEncoderFromNative(width: Int, height: Int, videoBitRate: Int, frameRate: Int): Int {
    return try {
      val ret = mSurfaceEncoder.start(width, height, videoBitRate, frameRate)
      mSurface = mSurfaceEncoder.getInputSurface()
      ret
    } catch (e: Exception) {
      e.printStackTrace()
      -1
    }
  }

  /**
   * 由c++回调回来
   * 获取h264数据
   * @param data h264 buffer, 由c++创建传递回来
   * @return 返回h264数据的大小, 返回0时数据无效
   */
  @Suppress("unused")
  private fun drainEncoderFromNative(data: ByteArray): Int {
    return mSurfaceEncoder.drainEncoder(data)
  }

  /**
   * 由c++回调回来
   * 获取当前编码的时间, 毫秒
   * @return 当前编码的时间 mBufferInfo.presentationTimeUs
   */
  @Suppress("unused")
  private fun getLastPresentationTimeUsFromNative(): Long {
    return mSurfaceEncoder.getLastPresentationTimeUs()
  }

  /**
   * 由c++回调回来
   * 获取编码器的Surface
   * @return 编码器的Surface mediaCodec.createInputSurface()
   */
  @Suppress("unused")
  private fun getEncodeSurfaceFromNative(): Surface? {
    return mSurface
  }

  /**
   * 由c++回调回来
   * 发送end of stream信号给编码器
   */
  @Suppress("unused")
  private fun signalEndOfInputStream() {
    mSurfaceEncoder.signalEndOfInputStream()
  }

  /**
   * 由c++回调回来
   * 关闭mediaCodec编码器
   */
  @Suppress("unused")
  private fun closeMediaCodecCalledFromNative() {
    mSurfaceEncoder.release()
  }

}