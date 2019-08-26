package com.trinity.record.exception

abstract class RecordingStudioException(detailMessage: String) : Exception(detailMessage) {

  companion object {
    private const val serialVersionUID = -3494098857987918589L
  }

}
