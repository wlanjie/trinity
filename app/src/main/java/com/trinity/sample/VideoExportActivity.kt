package com.trinity.sample

import android.os.Bundle
import android.view.View
import android.widget.VideoView
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.preference.PreferenceManager
import com.timqi.sectorprogressview.ColorfulRingProgressView
import com.trinity.core.TrinityCore
import com.trinity.editor.VideoExport
import com.trinity.editor.VideoExportInfo
import com.trinity.listener.OnExportListener
import java.io.File

/**
 * Created by wlanjie on 2019-07-30
 */
class VideoExportActivity : AppCompatActivity(), OnExportListener {

  private lateinit var mProgressBar: ColorfulRingProgressView
  private lateinit var mVideoView: VideoView
  private lateinit var mVideoExport: VideoExport

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_video_export)
    mProgressBar = findViewById(R.id.progress_view)
    mVideoView = findViewById(R.id.video_view)
    mVideoView.setOnPreparedListener {
      it.isLooping = true
    }

    val preferences = PreferenceManager.getDefaultSharedPreferences(this)
    val softCodecEncode = preferences.getBoolean("export_soft_encode", false)
    val softCodecDecode = preferences.getBoolean("export_soft_decode", false)
    val info = VideoExportInfo("/sdcard/export.mp4")
    info.mediaCodecDecode = !softCodecDecode
    info.mediaCodecEncode = !softCodecEncode
    val width = resources.displayMetrics.widthPixels
    val params = mVideoView.layoutParams as ConstraintLayout.LayoutParams
    params.width = width
    params.height = ((width * (info.height * 1.0f / info.width)).toInt())
    mVideoView.layoutParams = params
    mVideoExport = TrinityCore.createExport(this)
    mVideoExport.export(info, this)
  }

  override fun onExportProgress(progress: Float) {
//    println("progress: $progress")
    mProgressBar.percent = progress * 100
  }

  override fun onExportFailed(error: Int) {
  }

  override fun onExportCanceled() {
    val file = File("/sdcard/export.mp4")
    file.delete()
  }

  override fun onExportComplete() {
    mProgressBar.visibility = View.GONE
    mVideoView.setVideoPath("/sdcard/export.mp4")
    mVideoView.start()
  }

  override fun onDestroy() {
    super.onDestroy()
    mVideoExport.cancel()
  }
}