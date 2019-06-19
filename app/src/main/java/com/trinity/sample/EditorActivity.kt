package com.trinity.sample

import android.graphics.BitmapFactory
import android.os.Bundle
import android.os.Environment
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import androidx.annotation.NonNull
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.android.material.bottomsheet.BottomSheetDialog
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import com.trinity.editor.MediaClip
import com.trinity.editor.TimeRange
import com.trinity.editor.VideoEditor
import com.trinity.player.OnInitializedCallback
import com.trinity.player.TrinityPlayer
import com.trinity.sample.adapter.FilterAdapter
import com.trinity.sample.adapter.MusicAdapter
import com.trinity.sample.entity.Filter
import com.trinity.sample.view.SpacesItemDecoration
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.charset.Charset

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class EditorActivity : AppCompatActivity() {

  private var mFirst = true
  private var mVideoEditor = VideoEditor()
  private var mBottonSheetDialog: BottomSheetDialog ?= null

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_editor)

    openLog()
    val editorFile = externalCacheDir?.absolutePath + "/editor2.mp4"
    val file = File(editorFile)
    if (!file.exists()) {
      val stream = assets.open("editor2.mp4")
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

    val douyinMusicCacheFile = externalCacheDir?.absolutePath + "/douyin_quanjiwu.mp3"
    val douyinMusicFile = File(douyinMusicCacheFile)
    if (!douyinMusicFile.exists()) {
      val stream = assets.open("douyin_quanjiwu.mp3")
      val outputStream = FileOutputStream(douyinMusicFile)
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
//    mVideoEditor.insertClip(clip1)
    findViewById<Button>(R.id.play).setOnClickListener { mVideoEditor.resume() }
    findViewById<Button>(R.id.pause).setOnClickListener { mVideoEditor.pause() }
    findViewById<Button>(R.id.release).setOnClickListener { mVideoEditor.destroy() }
    findViewById<Button>(R.id.filter).setOnClickListener { showFilter() }
    findViewById<Button>(R.id.music).setOnClickListener { showMusic() }
    findViewById<Button>(R.id.export).setOnClickListener { export() }
//    mVideoEditor.play(true)
  }

  private fun export() {
    mVideoEditor.export("/sdcard/export.mp4", 540, 960, 25, 3000 * 1024, 44100, 1, 128 * 1000)
  }

  private fun showMusic() {
    val dialog = BottomSheetDialog(this)
    val view = LayoutInflater.from(this).inflate(R.layout.bottom_sheet_filter, null)
    val recyclerView = view as RecyclerView
    recyclerView.layoutManager = LinearLayoutManager(this, RecyclerView.VERTICAL, false)
    recyclerView.addItemDecoration(DividerItemDecoration(this, DividerItemDecoration.VERTICAL))
    recyclerView.adapter = MusicAdapter(this)
    dialog.setContentView(view)
    dialog.setCancelable(true)
    dialog.setCanceledOnTouchOutside(true)
    dialog.show()
  }

  private fun showFilter() {
    if (mBottonSheetDialog == null) {
      mBottonSheetDialog = BottomSheetDialog(this)
      mBottonSheetDialog?.setCancelable(true)
      mBottonSheetDialog?.setCanceledOnTouchOutside(true)
      val view = LayoutInflater.from(this).inflate(R.layout.bottom_sheet_filter, null)
      val recyclerView = view as RecyclerView
      recyclerView.layoutManager = LinearLayoutManager(this, LinearLayoutManager.HORIZONTAL, false)
      recyclerView.addItemDecoration(SpacesItemDecoration(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 8.0f, resources.displayMetrics).toInt()))
      val adapter = FilterAdapter(this) {
        setFilter(it)
      }
      recyclerView.adapter = adapter
      mBottonSheetDialog?.setContentView(view)
      mBottonSheetDialog?.window?.findViewById<View>(R.id.design_bottom_sheet)
        ?.setBackgroundResource(android.R.color.transparent)
      mBottonSheetDialog?.show()

      val delegateView = mBottonSheetDialog?.delegate?.findViewById<View>(com.google.android.material.R.id.design_bottom_sheet)
      val sheetBehavior = BottomSheetBehavior.from(delegateView)
      sheetBehavior.setBottomSheetCallback(object : BottomSheetBehavior.BottomSheetCallback() {
        override fun onStateChanged(@NonNull bottomSheet: View, newState: Int) {
          if (newState == BottomSheetBehavior.STATE_HIDDEN) {
            mBottonSheetDialog?.dismiss()
            sheetBehavior.state = BottomSheetBehavior.STATE_COLLAPSED
          }
        }

        //每次滑动都会触发
        override fun onSlide(@NonNull bottomSheet: View, slideOffset: Float) {
        }
      })
    } else {
      mBottonSheetDialog?.show()
    }
  }

  private fun setFilter(filter: Filter) {
    val configStream = assets.open("filter/${filter.config}")
    val size = configStream.available()
    val configBuffer = ByteArray(size)
    configStream.read(configBuffer)
    configStream.close()

    val configJson = JSONObject(String(configBuffer, Charset.forName("UTF-8")))
    val contentJson = configJson.optJSONObject("content")
    val keys = configJson.keys()

    val stream = assets.open("filter/${filter.lut}")
    val bitmap = BitmapFactory.decodeStream(stream)
    val buffer = ByteBuffer.allocate(bitmap.byteCount)
    bitmap.copyPixelsToBuffer(buffer)
    mVideoEditor.addFilter(buffer.array(), 0, Long.MAX_VALUE)
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