package com.trinity.record.exception

class AudioConfigurationException : RecordingStudioException("没有找到合适的音频配置") {
  companion object {
    private const val serialVersionUID = 491222852782937903L
  }
}
