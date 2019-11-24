package com.trinity.record.processor

class AudioRecordProcessor : RecordProcessor {
    private var handle: Long = 0

    override fun initAudioBufferSize(audioSampleRate: Int, audioBufferSize: Int) {
        handle = init(audioSampleRate, audioBufferSize)
    }

    override fun pushAudioBufferToQueue(audioSamples: ShortArray, audioSampleSize: Int) {
        pushAudioBufferToQueue(handle, audioSamples, audioSampleSize)
    }

    override fun flushAudioBufferToQueue() {
        flushAudioBufferToQueue(handle)
    }

    override fun destroy() {
        if (handle == 0L) {
            return
        }
        destroy(handle)
        handle = 0
    }

    private external fun init(audioSampleRate: Int, audioBufferSize: Int): Long
    private external fun flushAudioBufferToQueue(handle: Long)
    private external fun destroy(handle: Long)
    private external fun pushAudioBufferToQueue(
        handle: Long, audioSamples: ShortArray,
        audioSampleSize: Int
    ): Int
}
