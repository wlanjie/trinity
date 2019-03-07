package com.trinity.media

class SoundRecordController {

    /** 获取清唱时候的音量大小  */
    external fun getRecordVolume()

    /**
     * 初始化OpenSL 录音器
     *
     * @param minBufferSize
     * 每个buffer的大小，注意是short数组中元素的个数
     * @param sampleRate
     * 采样率
     * @param channels
     * 声道数
     * @param bitDepth
     * 位深度
     */
    external fun initMetaData(
        minBufferSize: Int, sampleRate: Int,
        channels: Int, bitDepth: Int
    ): Boolean

    /**
     * 初始化声音处理器
     * @param packetBufferSize
     */
    external fun initAudioRecorderProcessor(packetBufferSize: Int): Boolean

    /**
     * 销毁声音处理器
     */
    external fun destroyAudioRecorderProcessor()

    /**
     * 开始录音
     */
    external fun start(): Boolean

    /**
     * 停止录音
     */
    external fun stop()

    /************* 与打分相关的代码逻辑  */
    /**
     * 初始化打分处理器
     */
    external fun initScoring(sampleRate: Int, channels: Int, bitsDepth: Int, scoringType: Int)

    /**
     * 因为pitch scoring是异步初始化基频处理器的 用到重采样，初始化可能需要3秒钟左右的时间，这里如果在这3秒钟内退出，这里设置初始化完毕接着销毁
     */
    external fun setDestroyScoreProcessorFlag(destroyScoreProcessorFlag: Boolean)

    /**
     * 获取打分渲染的数据
     */
    external fun getRenderData(currentTimeMills: Long, meta: FloatArray)

    /**
     * 获取延迟（及第一次绘画的时间与最近的一个recordlevel的延时）
     */
    external fun getLatency(currentTimeMills: Long): Int

    /** 只有在清唱的时候才计算音量大小  */
    external fun enableUnAccom()
}
