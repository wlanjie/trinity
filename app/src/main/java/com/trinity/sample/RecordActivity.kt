package com.trinity.sample

import android.os.Bundle
import android.os.Environment
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import com.trinity.OnRecordingListener
import com.trinity.camera.TrinityPreviewView
import com.trinity.core.MusicInfo
import com.trinity.listener.OnRenderListener
import com.trinity.record.TrinityRecord
import com.trinity.record.VideoConfiguration

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class RecordActivity : AppCompatActivity(), OnRecordingListener, OnRenderListener {
  private lateinit var mRecord: TrinityRecord

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    setContentView(R.layout.activity_record)

    openLog()
    val preview = findViewById<TrinityPreviewView>(R.id.preview)
    val videoConfiguration = VideoConfiguration()

    mRecord = TrinityRecord(preview, videoConfiguration)
    mRecord.setOnRenderListener(this)

    findViewById<Button>(R.id.start_record)
      .setOnClickListener {
        mRecord.startEncode("/sdcard/a.mp4", 720, 1280,
          720 * 1280 * 20, 30, false, 44100, 1, 128 * 1000, Int.MAX_VALUE)
      }

    findViewById<Button>(R.id.stop_record)
      .setOnClickListener {
        mRecord.stopEncode()
      }

//    mRecord.setBackgroundMusic(MusicInfo("/sdcard/test.mp3"))
  }

  private fun openLog() {
    val logPath = Environment.getExternalStorageDirectory().absolutePath + "/trinity"
    if (BuildConfig.DEBUG) {
      Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
      Xlog.setConsoleLogOpen(true)
    } else {
      Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
      Xlog.setConsoleLogOpen(false)
    }
    Log.setLogImp(Xlog())
  }

  private fun closeLog() {
    Log.appenderClose()
  }

  override fun onRecording(currentTime: Int, duration: Int) {
  }

  override fun onPause() {
    super.onPause()
  }

  override fun onStop() {
    super.onStop()
  }

  override fun onDestroy() {
    super.onDestroy()
    closeLog()
  }

  override fun onSurfaceCreated() {
    println()
  }

  override fun onDrawFrame(textureId: Int, width: Int, height: Int, matrix: FloatArray): Int {
    return 0
  }

  override fun onSurfaceDestroy() {
    println()
  }
}