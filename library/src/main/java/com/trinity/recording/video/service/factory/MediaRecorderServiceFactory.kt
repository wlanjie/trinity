package com.trinity.recording.video.service.factory

import com.trinity.camera.PreviewScheduler
import com.trinity.recording.RecordingImplType
import com.trinity.recording.service.RecorderService
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl
import com.trinity.recording.video.service.MediaRecorderService
import com.trinity.recording.video.service.impl.MediaRecorderServiceImpl

class MediaRecorderServiceFactory private constructor() {

  fun getRecorderService(scheduler: PreviewScheduler, recordingImplType: RecordingImplType): MediaRecorderService {
    val recorderService = getAudioRecorderService(recordingImplType)
    return MediaRecorderServiceImpl(recorderService, scheduler)
  }

  fun getAudioRecorderService(
    recordingImplType: RecordingImplType
  ): RecorderService? {
    var recorderService: RecorderService? = null
    when (recordingImplType) {
      RecordingImplType.ANDROID_PLATFORM -> recorderService = AudioRecordRecorderServiceImpl.instance
    }
    return recorderService
  }

  companion object {
    @JvmStatic
    val instance = MediaRecorderServiceFactory()
  }

}
