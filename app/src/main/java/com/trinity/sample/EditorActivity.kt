package com.trinity.sample

import android.graphics.BitmapFactory
import android.os.Bundle
import android.os.Environment
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import com.trinity.editor.MediaClip
import com.trinity.editor.TimeRange
import com.trinity.editor.VideoEditor
import com.trinity.player.OnInitializedCallback
import com.trinity.player.TrinityPlayer
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class EditorActivity : AppCompatActivity() {

  private var mFirst = true
  private var mVideoEditor = VideoEditor()

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_editor)

    openLog()
    val editorFile = externalCacheDir?.absolutePath + "/editor.mp4"
    val file = File(editorFile)
    if (!file.exists()) {
      val stream = assets.open("editor.mp4")
      val outputStream = FileOutputStream(file)
      val buffer = ByteArray(2048)
      while (true) {
        val length = stream.read(buffer)
        if (length == -1) {
          break
        }
        outputStream.write(buffer)
      }
    }

    val xialuCacheFile = externalCacheDir?.absolutePath + "/xialu.mp4"
    val xialuFile = File(xialuCacheFile)
    if (!xialuFile.exists()) {
      val stream = assets.open("dy_xialu2.mp4")
      val outputStream = FileOutputStream(xialuFile)
      val buffer = ByteArray(2048)
      while (true) {
        val length = stream.read(buffer)
        if (length == -1) {
          break
        }
        outputStream.write(buffer)
      }
    }

    val surfaceView = findViewById<SurfaceView>(R.id.surface_view)
    mVideoEditor.setSurfaceView(surfaceView)
    val clip = MediaClip(file.absolutePath)
    mVideoEditor.insertClip(clip)

    val clip1 = MediaClip(xialuFile.absolutePath)
    mVideoEditor.insertClip(clip1)
    findViewById<Button>(R.id.play).setOnClickListener { mVideoEditor.resume() }
    findViewById<Button>(R.id.pause).setOnClickListener { mVideoEditor.pause() }
    findViewById<Button>(R.id.release).setOnClickListener { mVideoEditor.destroy() }
    mVideoEditor.play(true)

    val stream = assets.open("lut111.png")
    val bitmap = BitmapFactory.decodeStream(stream)
    val buffer = ByteBuffer.allocate(bitmap.byteCount)
    bitmap.copyPixelsToBuffer(buffer)
    mVideoEditor.addFilter(buffer.array())
  }

  override fun onPause() {
    super.onPause()
    mVideoEditor.pause()
  }

  override fun onResume() {
    super.onResume()
//    mVideoEditor.play(true)
    mVideoEditor.resume()
  }

  override fun onDestroy() {
    super.onDestroy()
    mVideoEditor.destroy()
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