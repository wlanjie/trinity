package com.trinity

import com.trinity.decoder.Mp3Decoder
import com.trinity.decoder.MusicDecoder
import com.trinity.recording.processor.AudioRecordProcessor
import com.trinity.recording.processor.RecordProcessor
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl

class SongStudio private constructor() {
    private var currentDecoder = MusicDecoder()
    private var audioRecorder = AudioRecordProcessor()

    /**
     * 由于各种手机的录音的minBufferSize不一样大小 所以得计算每个buffer的大小
     */
    var packetBufferTime = PacketBufferTimeEnum.TWENTY_PERCENT
        private set

    val mp3Decoder: Mp3Decoder
        get() {
            return currentDecoder
        }

    fun stopAudioRecord() {
        AudioRecordRecorderServiceImpl.getInstance().stop()
    }

    fun resetPacketBufferTime() {
        packetBufferTime = PacketBufferTimeEnum.TWENTY_PERCENT
    }

    fun upPacketBufferTimeLevel() {
        packetBufferTime = packetBufferTime.Upgrade()
    }


    fun getAudioRecorder(): RecordProcessor {
        return audioRecorder
    }

    companion object {
        @JvmStatic
        val instance = SongStudio()
    }

}
