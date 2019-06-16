package com.trinity.sample

import android.os.Bundle
import android.os.Environment
import androidx.appcompat.app.AppCompatActivity
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import com.trinity.player.AudioPlayer

class MusicActivity : AppCompatActivity() {

  private lateinit var mAudioPlayer: AudioPlayer

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    openLog()
    mAudioPlayer = AudioPlayer()
    mAudioPlayer.start(externalCacheDir?.absolutePath + "/douyin_quanjiwu.mp3")
  }

  override fun onPause() {
    super.onPause()
  }

  override fun onDestroy() {
    super.onDestroy()
    mAudioPlayer.stop()
    mAudioPlayer.release()
    closeLog()
  }

  private fun openLog() {
    val logPath = Environment.getExternalStorageDirectory().absolutePath + "/trinity"
    Xlog.open(true, Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", "")
    Xlog.setConsoleLogOpen(true)
    Log.setLogImp(Xlog())
  }

  private fun closeLog() {
    Log.appenderClose()
  }
}