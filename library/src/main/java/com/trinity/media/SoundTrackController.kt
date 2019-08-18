package com.trinity.media

class SoundTrackController {
    private var onCompletionListener: OnCompletionListener? = null
    /**
     * 获得伴奏的采样频率
     */
    external fun getAccompanySampleRate(): Int

    /**
     * 获得播放伴奏的当前时间
     */
    external fun getCurrentTimeMills() :Int

    /**
     * 判断当前播放器播放状态
     */
    external fun isPlaying(): Boolean

    interface OnCompletionListener {
        fun onCompletion()
    }

    fun setOnCompletionListener(onCompletionListener: OnCompletionListener) {
        this.onCompletionListener = onCompletionListener
    }

    /**
     * 设置播放文件地址，有可能是伴唱原唱都要进行设置
     */
    external fun setAudioDataSource(accompanyPath: String, percent: Float): Boolean

    /**
     * 播放伴奏
     */
    external fun play()

    /**
     * 设置播放的音量
     */
    external fun setVolume(volume: Float)

    /**
     * 如果有伴唱和原唱的时候，进行切换原唱伴唱
     */
    external fun setMusicSourceFlag(musicSourceFlag: Boolean)

    /**
     * 停止伴奏
     */
    external fun stop()

    /**
     * 暂停伴奏
     */
    external fun pause()

    /**
     * Seek伴奏
     */
    external fun seekToPosition(currentTimeSeconds: Float, delayTimeSeconds: Float)

    fun onCompletion() {
        onCompletionListener?.onCompletion()
    }

}
