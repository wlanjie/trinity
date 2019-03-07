package com.trinity.recording.service.factory

import com.trinity.recording.RecordingImplType
import com.trinity.recording.service.RecorderService
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl

class RecorderServiceFactory private constructor() {

  fun getRecorderService(recordingImplType: RecordingImplType): RecorderService? {
    var result: RecorderService? = null
    when (recordingImplType) {
      RecordingImplType.ANDROID_PLATFORM -> result = AudioRecordRecorderServiceImpl.instance
    }
    return result
  }

  companion object {
    @JvmStatic
    val instance = RecorderServiceFactory()
  }
}
