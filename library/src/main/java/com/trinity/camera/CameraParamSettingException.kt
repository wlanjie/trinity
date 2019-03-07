package com.trinity.camera

import com.trinity.recording.exception.RecordingStudioException

class CameraParamSettingException(msg: String) : RecordingStudioException(msg) {
    companion object {
        private val serialVersionUID = 1204332793566791080L
    }

}
