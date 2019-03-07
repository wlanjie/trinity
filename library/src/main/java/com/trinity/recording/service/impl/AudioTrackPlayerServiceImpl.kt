package com.trinity.recording.service.impl

import android.media.AudioManager
import com.trinity.player.AudioTrackPlayer
import com.trinity.recording.service.PlayerService

open class AudioTrackPlayerServiceImpl : PlayerService, PlayerService.OnCompletionListener {

    private var mediaPlayer: AudioTrackPlayer? = null
    private var mPlaying = false

    override val isPlaying
        get() = mPlaying

    override val accompanySampleRate: Int
        get() {
            try {
                if (mediaPlayer != null) {
                    return mediaPlayer?.accompanySampleRate ?: 0
                }
            } catch (e: IllegalStateException) {
                e.printStackTrace()
            }

            return -1
        }

    override val playerCurrentTimeMills: Int
        get() = mediaPlayer?.currentTimeMills ?: 0

    override val playedAccompanyTimeMills: Int
        get() = mediaPlayer?.playedAccompanyTimeMills ?: 0

    override val isPlayingAccompany: Boolean
        get() {
            var ret = false
            if (null != mediaPlayer) {
                ret = mediaPlayer?.isPlayingAccompany ?: false
            }
            return ret
        }

    init {
        try {
            if (mediaPlayer == null) {
                mediaPlayer = AudioTrackPlayer()
                mediaPlayer?.setAudioStreamType(AudioManager.STREAM_MUSIC)
                mediaPlayer?.setOnCompletionListener(this)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

    }

    override fun setAudioDataSource(vocalSampleRate: Int): Boolean {
        return mediaPlayer?.prepare(vocalSampleRate) ?: false
    }

    override fun start() {
        try {
            mPlaying = true
            mediaPlayer?.start()
        } catch (e: IllegalStateException) {
            e.printStackTrace()
        }

    }

    override fun stop() {
        try {
            mPlaying = false
            mediaPlayer?.stop()
        } catch (e: IllegalStateException) {
            e.printStackTrace()
        }

    }

    override fun onCompletion() {

    }

    override fun setVolume(volume: Float, accompanyMax: Float) {
        mediaPlayer?.setVolume(volume, accompanyMax)
    }

    override fun startAccompany(path: String) {
        mediaPlayer?.startAccompany(path)
    }

    override fun pauseAccompany() {
        mediaPlayer?.pauseAccompany()
    }

    override fun resumeAccompany() {
        mediaPlayer?.resumeAccompany()
    }

    override fun stopAccompany() {
        mediaPlayer?.stopAccompany()
    }
}
