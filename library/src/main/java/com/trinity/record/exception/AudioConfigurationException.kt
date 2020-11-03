package com.trinity.record.exception

class AudioConfigurationException : RecordingStudioException("No suitable audio configuration found") {
  companion object {
    private const val serialVersionUID = 491222852782937903L
  }
}
