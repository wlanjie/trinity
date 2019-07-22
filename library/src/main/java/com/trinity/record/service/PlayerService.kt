package com.trinity.record.service

/**
 * 播放器服务
 */
interface PlayerService {

    /** 获得伴唱的采样频率  */
    val accompanySampleRate: Int

    /** 是否在播放状态  */
    val isPlaying: Boolean
    /** 是否在播放伴奏的状态  */
    val isPlayingAccompany: Boolean

    /** 当前主播开播了多长时间 单位是毫秒  */
    val playerCurrentTimeMills: Int
    /** MiniPlayer获取播放伴奏的时间 单位是毫秒  */
    val playedAccompanyTimeMills: Int

    interface OnCompletionListener {
        fun onCompletion()
    }

    interface OnPreparedListener {
        fun onPrepared()
    }

    /** 初始化播放器  */
    fun setAudioDataSource(vocalSampleRate: Int): Boolean

    /** 主播开始发布的时候, 调用播放器的开始播放  */
    fun start()

    /** 主播停止发布的时候, 调用播放器的停止播放  */
    fun stop()

    /** 设置伴奏音量的大小  */
    fun setVolume(volume: Float, accompanyMax: Float)

    /** 播放一个伴奏  */
    fun startAccompany(path: String)

    /** MiniPlayer暂停播放  */
    fun pauseAccompany()

    /** MiniPlayer继续播放  */
    fun resumeAccompany()

    /** MiniPlayer停止播放  */
    fun stopAccompany()

}
