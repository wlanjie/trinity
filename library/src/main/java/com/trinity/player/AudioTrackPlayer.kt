package com.trinity.player

import android.content.ContentValues.TAG
import android.media.AudioFormat
import android.media.AudioTrack
import com.tencent.mars.xlog.Log
import com.trinity.decoder.Mp3Decoder
import com.trinity.record.service.PlayerService

/**
 * Create by wlanjie on 2019/3/7-下午7:47
 */

class AudioTrackPlayer {

  companion object {
    private const val BITS_PER_BYTE = 8
    private const val BITS_PER_CHANNEL = 16
    private const val CHANNEL_PER_FRAME = 2
    private const val channelConfig = AudioFormat.CHANNEL_OUT_STEREO
    private const val audioFormat = AudioFormat.ENCODING_PCM_16BIT
    private const val ACCOMPANY_QUEUE_USING_UP = -2
    private const val SILIENT_SAMPLE_TYPE = 0
    private const val ACCOMPANY_SAMPLE_TYPE = 1
  }

  private var mOnCompletionListener: PlayerService.OnCompletionListener? = null
  private var mSampleRateInHz = -1
  private var mDecoder: Mp3Decoder ?= null
  @Volatile private var mStop = false
  @Volatile private var mPlaying = false
  private var mDecodeBufferSize = 16 * 1024
  private var mAudioTrack: AudioTrack ?= null
  private var mPlayerThread: Thread ?= null

  // 用于计算Mini播放器的时间刷新
  private var mInPauseState = false
  private var audioTrackStartPlayBaseHeadPosition: Long = 0
  private var hasPlayedTimeMills: Long = 0
  private var pauseRequest = false
  private var isInSwitching = false
  private var isInStopping = false
  private var lastSampleType = SILIENT_SAMPLE_TYPE
  private var mLock = Object()
  var audioStreamType = -1

  fun setOnCompletionListener(l: PlayerService.OnCompletionListener) {
    mOnCompletionListener = l
  }

  fun prepare(vocalSampleRate: Int): Boolean {
    var result = true
    mSampleRateInHz = vocalSampleRate
      // TODO
//    mDecoder = SongStudio.instance.mp3Decoder
//    initMetaData()
//    val songStudio = SongStudio.instance
//    val percent = songStudio.packetBufferTime.percent
//    mDecoder?.init(percent, vocalSampleRate)
    result = initAudioTrack()
    if (result) {
      startPlayerThread()
    }
    return result
  }

  private fun initMetaData() {
    val byteCountPerSec = mSampleRateInHz * CHANNEL_PER_FRAME * BITS_PER_CHANNEL / BITS_PER_BYTE
//    val percent = SongStudio.instance.packetBufferTime.percent
//    mDecodeBufferSize = (byteCountPerSec / 2 * percent).toInt()
    mStop = false
    mPlaying = false
  }

  private fun initAudioTrack(): Boolean {
    var result = true
    val bufferSize = AudioTrack.getMinBufferSize(
      mSampleRateInHz,
      channelConfig, audioFormat
    )
    mAudioTrack = AudioTrack(
      this.audioStreamType, mSampleRateInHz,
      channelConfig, audioFormat, bufferSize,
      AudioTrack.MODE_STREAM
    )
    mAudioTrack?.setStereoVolume(1f, 1f)
    if (mAudioTrack?.state == AudioTrack.STATE_UNINITIALIZED) {
      result = false
    }
    return result
  }

  private fun startPlayerThread() {
    mPlayerThread = Thread(PlayerThread(this), "NativeMp3PlayerThread")
    mPlayerThread?.start()
  }

  fun start() {
    synchronized(AudioTrackPlayer::class.java) {
      try {
        mAudioTrack?.play()
        mPlaying = true
      } catch (t: Throwable) {
        t.printStackTrace()
      }
    }
  }

  fun stop() {
    if (!mStop) {
      if (mAudioTrack?.state != AudioTrack.STATE_UNINITIALIZED) {
        try {
          mAudioTrack?.stop()
        } catch (t: Throwable) {
          t.printStackTrace()
        }
      }
      mPlaying = true
      mStop = true
      synchronized(mLock) {
        mLock.notify()
      }
      try {
        mPlayerThread?.join()
        mPlayerThread = null
      } catch (t: Throwable) {
        t.printStackTrace()
      }

      closeAudioTrack()
      destroy()
    }
  }

  fun destroy() {
    mPlayerThread = null
    mAudioTrack = null
  }

  private fun closeAudioTrack() {
    try {
      if (mAudioTrack?.state != AudioTrack.STATE_UNINITIALIZED) {
        mAudioTrack?.release()
      }
    } catch (t: Throwable) {
      t.printStackTrace()
    }
  }

  private class PlayerThread(
    private val mAudioTrackPlayer: AudioTrackPlayer
  ) : Runnable {

    private val samples = ShortArray(mAudioTrackPlayer.mDecodeBufferSize)
    override fun run() {
      try {
        val extraSlientSampleSize = IntArray(1)
        val extraSampleType = IntArray(1)
        var sampleCount: Int
        var isPlayTemp: Boolean
        var sampleType: Int
        while (true) {
          synchronized(mAudioTrackPlayer.mLock) {
            if (mAudioTrackPlayer.pauseRequest) {
              mAudioTrackPlayer.mLock.wait()
            }
          }
          if (mAudioTrackPlayer.mStop) {
            break
          }

          extraSlientSampleSize[0] = 0
          extraSampleType[0] = SILIENT_SAMPLE_TYPE
          sampleCount = mAudioTrackPlayer.mDecoder?.readSamples(samples, extraSlientSampleSize, extraSampleType) ?: 0
          if (sampleCount == ACCOMPANY_QUEUE_USING_UP) {
            try {
              Thread.sleep(10)
            } catch (e: InterruptedException) {
              e.printStackTrace()
            }

            Log.i(TAG, "WARN : no play data")
            continue
          }

          while (true) {
            synchronized(AudioTrackPlayer::class.java) {
              isPlayTemp = mAudioTrackPlayer.mPlaying
            }
            if (isPlayTemp)
              break
            else
              Thread.yield()
          }

          sampleType = extraSampleType[0]
          if (SILIENT_SAMPLE_TYPE == mAudioTrackPlayer.lastSampleType && ACCOMPANY_SAMPLE_TYPE == sampleType) {
            if (mAudioTrackPlayer.mInPauseState) {
              //点击了继续播放按钮之后音乐播放
              mAudioTrackPlayer.audioTrackStartPlayBaseHeadPosition = mAudioTrackPlayer.mAudioTrack?.playbackHeadPosition?.toLong() ?: 0L
              mAudioTrackPlayer.mInPauseState = false
            } else {
              //开始播放伴奏之后音乐播放
              mAudioTrackPlayer.audioTrackStartPlayBaseHeadPosition = mAudioTrackPlayer.mAudioTrack?.playbackHeadPosition?.toLong() ?: 0L
              mAudioTrackPlayer.mInPauseState = false
              mAudioTrackPlayer.isInSwitching = false
              mAudioTrackPlayer.hasPlayedTimeMills = 0
            }
          }

          if (ACCOMPANY_SAMPLE_TYPE == mAudioTrackPlayer.lastSampleType && SILIENT_SAMPLE_TYPE == sampleType) {
            if (mAudioTrackPlayer.pauseRequest) {
              //点击了暂停按钮
              Log.i("problem", "点击了暂停按钮")
              val playedHeadPosition = (mAudioTrackPlayer.mAudioTrack?.playbackHeadPosition ?: 0) - mAudioTrackPlayer.audioTrackStartPlayBaseHeadPosition
              mAudioTrackPlayer.hasPlayedTimeMills += (playedHeadPosition.toFloat() * 1000 / mAudioTrackPlayer.mSampleRateInHz).toLong()
              mAudioTrackPlayer.mInPauseState = true
              mAudioTrackPlayer.pauseRequest = false
            } else if (mAudioTrackPlayer.isInSwitching) {
              //正在切换到一个新的伴奏
              Log.i("problem", "正在切换到一个新的伴奏")
            } else if (mAudioTrackPlayer.isInStopping) {
              //正在关闭一个伴奏
              mAudioTrackPlayer.isInStopping = false
              Log.i("problem", "正在关闭一个伴奏")
            } else {
              //播放结束了这个伴奏了
              Log.i("problem", "播放结束了这个伴奏了")
              mAudioTrackPlayer.mOnCompletionListener?.onCompletion()
            }
          }

          if (mAudioTrackPlayer.mAudioTrack?.state != AudioTrack.STATE_UNINITIALIZED) {
            if (extraSlientSampleSize[0] > 0) {
              val totalSize = extraSlientSampleSize[0] + sampleCount
              val playSamples = ShortArray(totalSize)
              System.arraycopy(samples, 0, playSamples, extraSlientSampleSize[0], sampleCount)
              mAudioTrackPlayer.mAudioTrack?.write(playSamples, 0, totalSize)
            } else {
              mAudioTrackPlayer.mAudioTrack?.write(samples, 0, sampleCount)
            }
          }
          mAudioTrackPlayer.lastSampleType = sampleType
        }

        mAudioTrackPlayer.mDecoder?.destroy()
      } catch (e: Exception) {
        e.printStackTrace()
      }
    }
  }

  fun setVolume(volume: Float, accompanyMax: Float) {
    mDecoder?.setAccompanyVolume(volume, accompanyMax)
  }

  fun getCurrentTimeMills(): Int {
    var currentTimeMills = 0
    try {
      val curPos = mAudioTrack?.playbackHeadPosition?.toLong() ?: 0
      currentTimeMills = (curPos.toFloat() * 1000 / mSampleRateInHz).toInt()
    } catch (e: Throwable) {
    }
    return currentTimeMills
  }

  fun startAccompany(path: String) {
    Log.i("trinity", "startAccompany path: ${path}")
    pauseRequest = false
    isInSwitching = true
    isInStopping = false
    hasPlayedTimeMills = 0
    mInPauseState = false
    mDecoder?.startAccompany(path)
  }

  fun pauseAccompany() {
    Log.i("trinity", "pauseAccompany")
    pauseRequest = true
    isInStopping = false
    mDecoder?.pauseAccompany()
  }

  fun resumeAccompany() {
    Log.i("trinity", "resumeAccompany")
    pauseRequest = false
    synchronized(mLock) {
      mLock.notify()
    }
    mDecoder?.resumeAccompany()
    isInStopping = false
  }

  fun stopAccompany() {
    isInStopping = true
    mDecoder?.stopAccompany()
  }

  fun getAccompanySampleRate(): Int {
    return mSampleRateInHz
  }

  fun getPlayedAccompanyTimeMills(): Int {
    var playedAccompanyTimeMills = 0
    try {
      val curHeadPosition = mAudioTrack?.playbackHeadPosition?.toLong() ?: 0
      playedAccompanyTimeMills = if (mInPauseState) {
        hasPlayedTimeMills.toInt()
      } else {
        if (SILIENT_SAMPLE_TYPE == lastSampleType || isInSwitching) {
          0
        } else {
          val subHeadPosition = curHeadPosition - audioTrackStartPlayBaseHeadPosition
          hasPlayedTimeMills.toInt() + (subHeadPosition.toFloat() * 1000 / mSampleRateInHz).toInt()
        }
      }
    } catch (e: Throwable) {
    }

    return playedAccompanyTimeMills
  }

  fun isPlayingAccompany(): Boolean {
    return ACCOMPANY_SAMPLE_TYPE == lastSampleType
  }
}