package com.trinity.record.service.factory

import com.trinity.record.RecordingImplType
import com.trinity.record.service.PlayerService
import com.trinity.record.service.impl.AudioTrackPlayerServiceImpl

class PlayerServiceFactory private constructor() {

  fun getPlayerService(
    onComletionListener: PlayerService.OnCompletionListener,
    recordingImplType: RecordingImplType
  ): PlayerService? {
    var result: PlayerService? = null
    when (recordingImplType) {
      RecordingImplType.ANDROID_PLATFORM -> result = object : AudioTrackPlayerServiceImpl() {
        override fun onCompletion() {
          onComletionListener.onCompletion()
        }
      }
    }
    return result
  }

  companion object {
    @JvmStatic
    val instance = PlayerServiceFactory()
  }
}
