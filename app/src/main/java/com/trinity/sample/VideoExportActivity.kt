package com.trinity.sample

import android.os.Bundle
import android.view.View
import android.widget.VideoView
import androidx.appcompat.app.AppCompatActivity
import com.timqi.sectorprogressview.ColorfulRingProgressView
import com.trinity.editor.VideoExport
import com.trinity.listener.OnExportListener

/**
 * Created by wlanjie on 2019-07-30
 */
class VideoExportActivity : AppCompatActivity(), OnExportListener {

  private lateinit var mProgressBar: ColorfulRingProgressView
  private lateinit var mVideoView: VideoView

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_video_export)
    mProgressBar = findViewById(R.id.progress_view)
    mVideoView = findViewById(R.id.video_view)
    mVideoView.setOnPreparedListener {
      it.isLooping = true
    }
    val export = VideoExport(this)
    export.export("/sdcard/export.mp4", 544, 960, 25, 3000 * 1024, 44100, 1, 128 * 1000, this)
  }

  override fun onExportProgress(progress: Float) {
    println("progress: $progress")
    mProgressBar.percent = progress * 100
  }

  override fun onExportFailed(error: Int) {
  }

  override fun onExportCanceled() {
  }

  override fun onExportComplete() {
    mProgressBar.visibility = View.GONE
    mVideoView.setVideoPath("/sdcard/export.mp4")
    mVideoView.start()
  }
}