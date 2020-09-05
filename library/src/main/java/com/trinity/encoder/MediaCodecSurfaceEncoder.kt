/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
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
 */

package com.trinity.encoder

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.media.MediaFormat
import android.view.Surface
import com.tencent.mars.xlog.Log
import kotlin.experimental.and

class MediaCodecSurfaceEncoder {

  companion object {
    private const val TAG = "trinity"
    private const val MIME_TYPE = "video/avc"
    private const val FRAME_INTERVAL = 1
    private const val IDR = 5
    private const val SPS = 7
    private const val TIMEOUT_US: Long = 5000
    private val BYTES_HEADER = byteArrayOf(0, 0, 0, 1)
  }

  private var mEncoder: MediaCodec ?= null
  private var mInputSurface: Surface ?= null
  private val mBufferInfo = MediaCodec.BufferInfo()
  private var mLastPresentationTimeUs: Long = 0

  fun start(width: Int, height: Int, bitRate: Int, frameRate: Int): Int {
    if (width <= 0 || height <= 0 || bitRate <= 0 || frameRate <= 0) {
      Log.e(TAG, "width: $width height: $height, bitRate: $bitRate frameRate: $frameRate")
      return -1
    }
    val format = MediaFormat.createVideoFormat(MIME_TYPE, width, height)
    format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
    format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate)
    format.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate)
    format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, FRAME_INTERVAL)
    try {
      mEncoder = MediaCodec.createEncoderByType(MIME_TYPE)
      mEncoder?.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
      mInputSurface = mEncoder?.createInputSurface()
      mEncoder?.start()
    } catch (e: Exception) {
      e.printStackTrace()
      Log.e("trinity", e.message)
      return -1
    }
    return 0
  }

  fun getInputSurface(): Surface? {
    return mInputSurface
  }

  fun getLastPresentationTimeUs(): Long {
    return mLastPresentationTimeUs
  }

  fun signalEndOfInputStream() {
    mEncoder?.signalEndOfInputStream()
  }

  fun release() {
    mLastPresentationTimeUs = 0;
    mEncoder?.stop()
    mEncoder?.release()
    mEncoder = null
  }

  private fun isH264StartCode(code: Int): Boolean {
    return code == 0x01
  }

  private fun isH264StartCodePrefix(code: Int): Boolean {
    return code == 0x00
  }

  private fun findAnnexbStartCodecIndex(data: ByteArray, offset: Int): Int {
    var index = offset
    var cursor = offset
    while (cursor < data.size) {
      val code = data[cursor++].toInt()
      if (isH264StartCode(code) && cursor >= 3) {
        val firstPrefixCode = data[cursor - 3].toInt()
        val secondPrefixCode = data[cursor - 2].toInt()
        if (isH264StartCodePrefix(firstPrefixCode) && isH264StartCodePrefix(secondPrefixCode)) {
          break
        }
      }
    }
    index = cursor
    return index
  }

  private fun splitIDRFrame(h264Data: ByteArray): ByteArray {
    var result = h264Data
    var offset = 0
    do {
      offset = findAnnexbStartCodecIndex(h264Data, offset)
      val naluSpecIndex = offset
      if (naluSpecIndex < h264Data.size) {
        val naluType = (h264Data[naluSpecIndex] and 0x1F).toInt()
        if (naluType == IDR) {
          val idrFrameLength = h264Data.size - naluSpecIndex + 4
          result = ByteArray(idrFrameLength)
          System.arraycopy(BYTES_HEADER, 0, result, 0, 4)
          System.arraycopy(h264Data, naluSpecIndex, result, 4, idrFrameLength - 4)
          break
        }
      }
    } while (offset < h264Data.size)
    return result
  }

  fun drainEncoder(data: ByteArray): Int {
    var length = 0
    val encoderOutputBuffers = mEncoder?.outputBuffers ?: return 0
    val status = mEncoder?.dequeueOutputBuffer(mBufferInfo, TIMEOUT_US) ?: return 0
    if (status >= 0) {
      val encodeData = encoderOutputBuffers[status]
      // config data sps/pps
      if ((mBufferInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
        // The codec config_ data was pulled out when we got the
        // INFO_OUTPUT_FORMAT_CHANGED status. The MediaMuxer won't
        // accept
        // a single big blob -- it wants separate csd-0/csd-1 chunks --
        // so simply saving this off won't work.
        if (mBufferInfo.size != 0) {
          encodeData.position(mBufferInfo.offset)
          encodeData.limit(mBufferInfo.offset + mBufferInfo.size)
          length = mBufferInfo.size
          encodeData.get(data, 0, length)
          mBufferInfo.size = 0
        }
      }
      if (mBufferInfo.presentationTimeUs >= mLastPresentationTimeUs) {
        if (mBufferInfo.size != 0) {
          encodeData.position(mBufferInfo.offset)
          encodeData.limit(mBufferInfo.offset + mBufferInfo.size)
          mLastPresentationTimeUs = mBufferInfo.presentationTimeUs
          length = mBufferInfo.size
          encodeData.get(data, 0, mBufferInfo.size)
          val type = (data[4] and 0x1F).toInt()
          if (type == SPS) {
            // 某些手机I帧前面会附带上SPS和PPS，剥离掉SPS和PPS，仅使用I帧
            val idrFrame = splitIDRFrame(data)
            System.arraycopy(idrFrame, 0, data, 0, idrFrame.size)
            length = idrFrame.size
          }
        }
      }
      mEncoder?.releaseOutputBuffer(status, false)
      if ((mBufferInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
        // end of stream, break encode data
        return -1
      }
    }
    return length
  }
}