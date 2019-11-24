package com.trinity.decoder

import android.media.MediaCodec
import android.media.MediaFormat
import android.view.Surface
import java.io.IOException
import java.lang.Exception
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MediaCodecDecode {

  private var mMediaCodec: MediaCodec? = null
  private var mFormat: MediaFormat? = null
  private var mOutputSurface: Surface? = null
  private var mOutputBufferInfo = MediaCodec.BufferInfo()
  private val mBuffer = ByteBuffer.allocateDirect(16)
  private val mChangeBuffer = ByteBuffer.allocateDirect(12)

  init {
    mBuffer.order(ByteOrder.BIG_ENDIAN)
    mChangeBuffer.order(ByteOrder.BIG_ENDIAN)
  }

  fun init(codecName: String, width: Int, height: Int, csd0: ByteBuffer?, csd1: ByteBuffer?) {
    try {
      mMediaCodec = MediaCodec.createDecoderByType(codecName)
      mFormat = MediaFormat()
      mFormat?.setString(MediaFormat.KEY_MIME, codecName)
      mFormat?.setInteger(MediaFormat.KEY_WIDTH, width)
      mFormat?.setInteger(MediaFormat.KEY_HEIGHT, height)
      when (codecName) {
        "video/avc" -> {
          if (csd0 != null && csd1 != null) {
            mFormat?.setByteBuffer("csd-0", csd0)
            mFormat?.setByteBuffer("csd-1", csd1)
          }
        }

        "video/hevc" -> {
          if (csd0 != null) {
            mFormat?.setByteBuffer("csd-0", csd0)
          }
        }

        "video/mp4v-es" -> {
          mFormat?.setByteBuffer("csd-0", csd0)
        }

        "video/3gpp" -> {
          mFormat?.setByteBuffer("csd-0", csd0)
        }
      }
      mMediaCodec?.configure(mFormat, mOutputSurface, null, 0)
      mMediaCodec?.start()
    } catch (e: IOException) {
      e.printStackTrace()
    }
  }

  fun stop() {
    mMediaCodec?.stop()
  }

  fun flush() {
    mMediaCodec?.flush()
  }

  fun dequeueInputBuffer(timeout: Long): Int {
    return mMediaCodec?.dequeueInputBuffer(timeout) ?: -1
  }

  fun getInputBuffer(id: Int): ByteBuffer? {
    return mMediaCodec?.inputBuffers?.get(id)
  }

  fun queueInputBuffer(id: Int, size: Int, pts: Long, flags: Int) {
    mMediaCodec?.queueInputBuffer(id, 0, size, pts, flags)
  }

  fun dequeueOutputBufferIndex(timeout: Long): ByteBuffer? {
    val id = mMediaCodec?.dequeueOutputBuffer(mOutputBufferInfo, timeout) ?: -1
    mBuffer.position(0)
    mBuffer.putInt(id)
    if (id >= 0) {
      mBuffer.putInt(mOutputBufferInfo.offset)
      mBuffer.putLong(mOutputBufferInfo.presentationTimeUs)
    }
    return mBuffer
  }

  fun releaseOutputBuffer(id: Int) {
    try {
      mMediaCodec?.releaseOutputBuffer(id, true)
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  fun formatChange(): ByteBuffer {
    val newFormat = mMediaCodec?.outputFormat
    val width = newFormat?.getInteger(MediaFormat.KEY_WIDTH) ?: -1
    val height = newFormat?.getInteger(MediaFormat.KEY_HEIGHT) ?: -1
    val colorFormat = newFormat?.getInteger(MediaFormat.KEY_COLOR_FORMAT) ?: -1
    mChangeBuffer.position(0)
    mChangeBuffer?.putInt(width)
    mChangeBuffer?.putInt(height)
    mChangeBuffer?.putInt(colorFormat)
    return mChangeBuffer
  }

  fun getOutputBuffer(id: Int): ByteBuffer? {
    return mMediaCodec?.outputBuffers?.get(id)
  }

  fun release() {
    mMediaCodec?.release()
    mMediaCodec = null
    mFormat = null
  }

  fun setOutputSurface(surface: Surface?) {
    mOutputSurface = surface
  }
}