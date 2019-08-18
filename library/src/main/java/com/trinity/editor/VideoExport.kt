package com.trinity.editor

import android.content.Context
import com.trinity.listener.OnExportListener
import com.trinity.util.Trinity

/**
 * Created by wlanjie on 2019-07-30
 */
class VideoExport(private val context: Context) {

  private var mHandle = create()
  private var mListener: OnExportListener ?= null

  fun export(
      path: String,
      width: Int,
      height: Int,
      frameRate: Int,
      videoBitRate: Int,
      sampleRate: Int,
      channelCount: Int,
      audioBitRate: Int,
      l: OnExportListener
  ): Int {
    mListener = l
    val resourcePath = context.externalCacheDir?.absolutePath + "/resource.json"
    return export(mHandle, resourcePath, path, width, height, frameRate, videoBitRate, sampleRate, channelCount, audioBitRate)
  }

  fun cancel() {

  }

  fun release() {
    if (mHandle <= 0) {
      return
    }
    release(mHandle)
    mHandle = 0
  }

  private external fun create(): Long

  private external fun export(handle: Long, resourcePath: String, path: String, width: Int, height: Int, frameRate: Int, videoBitRate: Int, sampleRate: Int, channelCount: Int, audioBitRate: Int): Int

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
}