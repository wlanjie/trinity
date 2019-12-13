package com.trinity.decoder

import android.graphics.SurfaceTexture
import android.media.MediaCodec
import android.media.MediaFormat
import android.view.Surface
import com.tencent.mars.xlog.Log
import java.io.IOException
import java.lang.Exception
import java.nio.ByteBuffer
import java.nio.ByteOrder

class MediaCodecDecode {

  private var mMediaCodec: MediaCodec? = null
  private var mFormat: MediaFormat? = null
  private var mSurfaceTexture: SurfaceTexture ?= null
  private var mOutputSurface: Surface? = null
  private var mOutputBufferInfo = MediaCodec.BufferInfo()
  private val mBuffer = ByteBuffer.allocateDirect(16)
  private val mChangeBuffer = ByteBuffer.allocateDirect(12)
  private val mMatrix = FloatArray(16)

  init {
    mBuffer.order(ByteOrder.BIG_ENDIAN)
    mChangeBuffer.order(ByteOrder.BIG_ENDIAN)
  }

  fun start(textureId: Int, codecName: String, width: Int, height: Int,
           csd0: ByteBuffer?, csd1: ByteBuffer?) {
    try {
      Log.i("trinity", "enter MediaCodec Start textureId: $textureId codecName: $codecName width: $width height: $height")
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
      mSurfaceTexture = SurfaceTexture(textureId)
      mOutputSurface = Surface(mSurfaceTexture)
      mMediaCodec?.configure(mFormat, mOutputSurface, null, 0)
      mMediaCodec?.start()
    } catch (e: IOException) {
      e.printStackTrace()
    }
    Log.i("trinity", "leave MediaCodec Start")
  }

  fun stop() {
    Log.i("trinity", "enter MediaCodec Stop")
    flush()
    mMediaCodec?.stop()
    mMediaCodec?.release()
    mSurfaceTexture?.release()
    mSurfaceTexture = null
    mOutputSurface?.release()
    mOutputSurface = null
    mMediaCodec = null
    mFormat = null
    Log.i("trinity", "leave MediaCodec Stop")
  }

  fun flush() {
//    while (true) {
//      val id = mMediaCodec?.dequeueOutputBuffer(mOutputBufferInfo, 0) ?: -1
//      if (id < 0) {
//        break
//      }
//      Log.e("trinity", "flush id: $id")
//      mMediaCodec?.releaseOutputBuffer(id, true)
//    }
    mMediaCodec?.flush()
  }

  fun dequeueInputBuffer(timeout: Long): Int {
    return mMediaCodec?.dequeueInputBuffer(timeout) ?: -10
  }

  fun getInputBuffer(id: Int): ByteBuffer? {
    return mMediaCodec?.inputBuffers?.get(id)
  }

  fun queueInputBuffer(id: Int, size: Int, pts: Long, flags: Int) {
//    Log.e("trinity", "id: $id size: $size pts: $pts")
    mMediaCodec?.queueInputBuffer(id, 0, size, pts, flags)
  }

  fun dequeueOutputBufferIndex(timeout: Long): ByteBuffer? {
    val id = mMediaCodec?.dequeueOutputBuffer(mOutputBufferInfo, timeout) ?: -1000
    mBuffer.position(0)
    mBuffer.putInt(id)
//    Log.e("trinity", "dequeueOutputBufferIndex id: $id")
    if (id >= 0) {
      mBuffer.putInt(mOutputBufferInfo.offset)
      mBuffer.putLong(mOutputBufferInfo.presentationTimeUs)
    }
    return mBuffer
  }

  fun releaseOutputBuffer(id: Int) {
    try {
//      Log.e("trinity", "releaseOutputBuffer: $id")
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

  fun updateTexImage() {
    mSurfaceTexture?.updateTexImage()
  }

  fun getTransformMatrix(): FloatArray {
    mSurfaceTexture?.getTransformMatrix(mMatrix)
    return mMatrix
  }
}