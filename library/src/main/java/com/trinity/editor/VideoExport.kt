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
import com.trinity.BuildConfig
import com.trinity.encoder.MediaCodecSurfaceEncoder
import com.trinity.listener.OnExportListener
import com.trinity.util.Trinity
import java.text.SimpleDateFormat
import java.util.*

/**
 * Created by wlanjie on 2019-07-30
 */
class TrinityVideoExport(private val context: Context) : VideoExport {

  private var mHandle = create()
  // Hardcoded object
  private var mSurfaceEncoder = MediaCodecSurfaceEncoder()
  // Surface Object
  private var mSurface: Surface ?= null
  private var mListener: OnExportListener ?= null

  private fun getNow(): String {
    return if (android.os.Build.VERSION.SDK_INT >= 24) {
      SimpleDateFormat("yyyy-MM-dd-HH-mm", Locale.US).format(Date())
    } else {
      val tms = Calendar.getInstance()
      tms.get(Calendar.YEAR).toString() + "-" +
              tms.get(Calendar.MONTH).toString() + "-" +
              tms.get(Calendar.DAY_OF_MONTH).toString() + "-" +
              tms.get(Calendar.HOUR_OF_DAY).toString() + "-" +
              tms.get(Calendar.MINUTE).toString()
    }
  }

  override fun export(info: VideoExportInfo, l: OnExportListener): Int {
    mListener = l
    val tag = "trinity-export-" + BuildConfig.VERSION_NAME + "-" + getNow()
    val resourcePath = context.externalCacheDir?.absolutePath + "/resource.json"
    return export(mHandle, resourcePath, info.path,
      info.width, info.height, info.frameRate, info.videoBitRate,
      info.sampleRate, info.channelCount, info.audioBitRate,
      info.mediaCodecDecode, info.mediaCodecEncode, tag)
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
                              audioBitRate: Int, mediaCodecDecode: Boolean, mediaCodecEncode: Boolean,
                              tag: String): Int

  private external fun cancel(handle: Long)

  private external fun release(handle: Long)

  /**
   * Export progress, call back by C++, do not change
   * @param progress Export progress, the progress ratio is 0.0-1.0
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
   * After the export fails, it will be called back by C++
   * @param error error code
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
   * After the export is completed, it will be called back by C++
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
   * After the export is cancelled, it will be called back by c++
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
   * Call back by c++
   * Create hard code
   * @param width The width of the recorded video
   * @param height High of recorded video
   * @param videoBitRate Bit rate of recorded video
   * @param frameRate Frame rate of recorded video
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
   * Call back by c++
   * Get h264 data
   * @param data h264 buffer, Created by c++ and passed back
   * @return Returns the size of the h264 data, the data is invalid when 0 is returned
   */
  @Suppress("unused")
  private fun drainEncoderFromNative(data: ByteArray): Int {
    return mSurfaceEncoder.drainEncoder(data)
  }

  /**
   * Call back by c++
   * Get the current encoding time, milliseconds
   * @return Current encoding time mBufferInfo.presentationTimeUs
   */
  @Suppress("unused")
  private fun getLastPresentationTimeUsFromNative(): Long {
    return mSurfaceEncoder.getLastPresentationTimeUs()
  }

  /**
   * Call back by c++
   * Get the Surface of the encoder
   * @return Encoder Surface mediaCodec.createInputSurface()
   */
  @Suppress("unused")
  private fun getEncodeSurfaceFromNative(): Surface? {
    return mSurface
  }

  /**
   * Call back by c++
   * send end of stream Signal to encoder
   */
  @Suppress("unused")
  private fun signalEndOfInputStream() {
    mSurfaceEncoder.signalEndOfInputStream()
  }

  /**
   * Call back by c++
   * Close mediaCodec encoder
   */
  @Suppress("unused")
  private fun closeMediaCodecCalledFromNative() {
    mSurfaceEncoder.release()
  }

}