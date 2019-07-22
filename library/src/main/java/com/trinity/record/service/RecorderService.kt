package com.trinity.record.service

import com.trinity.record.exception.AudioConfigurationException
import com.trinity.record.exception.StartRecordingException

interface RecorderService {

    /**
     * 获得后台处理数据部分的buffer大小
     */
    val audioBufferSize: Int

    val sampleRate: Int

    /**
     * 清唱的时候获取当前音高 来渲染音量动画
     */
    val recordVolume: Int

    val isPaused: Boolean

    /**
     * 初始化录音器的硬件部分
     */
    @Throws(AudioConfigurationException::class)
    fun init()

    /**
     * 初始化我们后台处理数据部分
     */
    fun initAudioRecorderProcessor(): Boolean

    /**
     * 销毁我们的后台处理数据部分
     */
    fun destroyAudioRecorderProcessor()

    /**
     * 开始录音
     */
    @Throws(StartRecordingException::class)
    fun start()

    /**
     * 结束录音
     */
    fun stop()

    fun enableUnAccom()

    fun pause()

    fun continueRecord()

}
