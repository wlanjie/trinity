package com.trinity.recording.service.impl

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import com.trinity.PacketBufferTimeEnum
import com.trinity.SongStudio
import com.trinity.recording.exception.AudioConfigurationException
import com.trinity.recording.exception.StartRecordingException
import com.trinity.recording.processor.RecordProcessor
import com.trinity.recording.service.PlayerService
import com.trinity.recording.service.RecorderService

class AudioRecordRecorderServiceImpl private constructor() : RecorderService {
  private var audioRecord: AudioRecord? = null
  private var recordThread: Thread? = null
  private var recordProcessor: RecordProcessor? = null
  private var mRecordVolume = 0

  override val recordVolume: Int
    get() {
      return mRecordVolume
    }

  private var bufferSizeInBytes = 0
  private var bufferSizeInShorts = 0
  private var mRecording = false
  private var mPaused = false
  private var isUnAccom = false
  private var playerService: PlayerService? = null

  //音频清唱会暂停
  override val isPaused: Boolean
    get() = mPaused


  override val audioBufferSize: Int
    get() {
      val songStudio = SongStudio.instance
      val packetBufferTime = songStudio.packetBufferTime
      val percent = packetBufferTime.percent
      return (SAMPLE_RATE_IN_HZ * percent).toInt()
    }

  private val playCurrentTimeMills: Int
    get() {
      return playerService?.playerCurrentTimeMills ?: 0
    }

  override val sampleRate: Int
    get() = SAMPLE_RATE_IN_HZ

  @Throws(AudioConfigurationException::class)
  override fun initMetaData() {
    if (null != audioRecord) {
      audioRecord?.release()
    }
    try {
      // 首先利用我们标准的44_1K的录音频率初是录音器
      bufferSizeInBytes = AudioRecord.getMinBufferSize(
        SAMPLE_RATE_IN_HZ,
        CHANNEL_CONFIGURATION, AUDIO_FORMAT
      )
      audioRecord = AudioRecord(
        AUDIO_SOURCE, SAMPLE_RATE_IN_HZ,
        CHANNEL_CONFIGURATION, AUDIO_FORMAT, bufferSizeInBytes
      )
    } catch (e: Exception) {
      e.printStackTrace()
    }

    // 如果初始化不成功的话,则降低为16K的采样率来初始化录音器
    if (audioRecord == null || audioRecord?.state != AudioRecord.STATE_INITIALIZED) {
      try {
        SAMPLE_RATE_IN_HZ = 16000
        bufferSizeInBytes = AudioRecord.getMinBufferSize(
          SAMPLE_RATE_IN_HZ, CHANNEL_CONFIGURATION, AUDIO_FORMAT
        )
        audioRecord = AudioRecord(
          AUDIO_SOURCE, SAMPLE_RATE_IN_HZ,
          CHANNEL_CONFIGURATION, AUDIO_FORMAT, bufferSizeInBytes
        )
      } catch (e: Exception) {
        e.printStackTrace()
      }

    }
    if (audioRecord == null || audioRecord?.state != AudioRecord.STATE_INITIALIZED) {
      throw AudioConfigurationException()
    }
    bufferSizeInShorts = bufferSizeInBytes / 2
  }

  @Throws(StartRecordingException::class)
  override fun start() {
    if (audioRecord != null && audioRecord!!.state == AudioRecord.STATE_INITIALIZED) {
      try {
        audioRecord?.startRecording()
      } catch (e: Exception) {
        throw StartRecordingException()
      }

    } else {
      throw StartRecordingException()
    }
    mRecording = true
    mPaused = false
    recordThread = Thread(RecordThread(), "RecordThread")
    try {
      recordThread?.start()
    } catch (e: Exception) {
      throw StartRecordingException()
    }

  }

  override fun initAudioRecorderProcessor(): Boolean {
    var result = true
    val songStudio = SongStudio.instance
    songStudio.resetPacketBufferTime()
    recordProcessor = songStudio.getAudioRecorder()
    var packetBufferTime: PacketBufferTimeEnum
    var percent: Float
    while (true) {
      packetBufferTime = songStudio.packetBufferTime
      percent = packetBufferTime.percent
      if (packetBufferTime == PacketBufferTimeEnum.ERROR_STATE) {
        result = false
        break
      }
      if (SAMPLE_RATE_IN_HZ * percent < bufferSizeInShorts) {
        songStudio.upPacketBufferTimeLevel()
      } else {
        break
      }
    }
    recordProcessor?.initAudioBufferSize(SAMPLE_RATE_IN_HZ, (SAMPLE_RATE_IN_HZ * percent).toInt())
    return result
  }

  override fun destroyAudioRecorderProcessor() {
    recordProcessor?.destroy()
  }

  override fun stop() {
    try {
      if (audioRecord != null) {
        mRecording = false
        mPaused = false
        try {
          recordThread?.join()
          recordThread = null
        } catch (e: InterruptedException) {
          e.printStackTrace()
        }

        releaseAudioRecord()
      }
    } catch (e: Exception) {
      e.printStackTrace()
    }

  }

  private fun releaseAudioRecord() {
    if (audioRecord?.state == AudioRecord.STATE_INITIALIZED) {
      audioRecord?.stop()
    }
    audioRecord?.release()
    audioRecord = null
  }

  override fun enableUnAccom() {
    isUnAccom = true
  }

  internal inner class RecordThread : Runnable {

    override fun run() {
      val scoringBufferMaxSize = bufferSizeInShorts
      val audioSamples = ShortArray(scoringBufferMaxSize)
//            val offset = (scoringBufferMaxSize.toLong() * 1000 / SAMPLE_RATE_IN_HZ).toInt()
      while (mRecording) {
        val localPaused = mPaused
        //				android.util.Log.d(TAG, "RecordThread run() localPaused="+localPaused+"  isUnAccom="+isUnAccom);
        if (isUnAccom && localPaused) { //只有是清唱才能暂停录制
          continue
        }
        val audioSampleSize = getAudioRecordBuffer(
          scoringBufferMaxSize, audioSamples
        )
        if (audioSampleSize > 0) {
          if (isUnAccom) {
            val x = Math.abs(audioSamples[0].toInt()).toFloat() / java.lang.Short.MAX_VALUE
            mRecordVolume = Math.round((2 * x - x * x) * 9)
            // Log.i("problem", "recordVolume"+recordVolume);
          }
          recordProcessor!!.pushAudioBufferToQueue(
            audioSamples,
            audioSampleSize
          )
        }
      }
      recordProcessor!!.flushAudioBufferToQueue()
    }
  }

  private fun getAudioRecordBuffer(
    scoringBufferMaxSize: Int,
    audioSamples: ShortArray
  ): Int {
    return audioRecord?.read(
      audioSamples, 0,
      scoringBufferMaxSize
    ) ?: 0
  }

  override fun pause() {
    mPaused = true
  }

  override fun continueRecord() {
    mPaused = false
  }

  companion object {
    var SAMPLE_RATE_IN_HZ = 44100
    private const val AUDIO_SOURCE = MediaRecorder.AudioSource.MIC
    private const val CHANNEL_CONFIGURATION = AudioFormat.CHANNEL_IN_MONO
    private const val AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT
    @JvmStatic
    val instance = AudioRecordRecorderServiceImpl()
  }

}
