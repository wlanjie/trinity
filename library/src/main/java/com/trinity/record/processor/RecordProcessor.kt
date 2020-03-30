package com.trinity.record.processor

interface RecordProcessor {
    /**
     * 初始化AudioQueue里面的每个packet的大小
     */
    fun initAudioBufferSize(audioSampleRate: Int, audioBufferSize: Int, speed: Float)

    fun destroy()
    /**
     * 将audioBuffer放入队列中
     */
    fun pushAudioBufferToQueue(audioSamples: ShortArray, audioSampleSize: Int)

    /**
     * 把还未放入AudioQueue的buffer全部刷出到queue里面
     */
    fun flushAudioBufferToQueue()

}
