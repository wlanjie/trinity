package com.trinity.sample

import android.Manifest
import android.annotation.SuppressLint
import android.content.Intent
import android.content.SharedPreferences
import android.graphics.PointF
import android.os.Bundle
import android.view.View
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.RadioGroup
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.transaction
import androidx.preference.PreferenceManager
import com.github.florent37.runtimepermission.kotlin.askPermission
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.tencent.mars.xlog.Log
import com.trinity.OnRecordingListener
import com.trinity.camera.CameraCallback
import com.trinity.camera.Flash
import com.trinity.camera.TrinityPreviewView
import com.trinity.core.Frame
import com.trinity.core.MusicInfo
import com.trinity.listener.OnRenderListener
import com.trinity.record.PreviewResolution
import com.trinity.record.Speed
import com.trinity.record.TrinityRecord
import com.trinity.sample.entity.MediaItem
import com.trinity.sample.fragment.MediaFragment
import com.trinity.sample.fragment.MusicFragment
import com.trinity.sample.fragment.SettingFragment
import com.trinity.sample.view.LineProgressView
import com.trinity.sample.view.RecordButton
import com.trinity.sample.view.foucs.AutoFocusTrigger
import com.trinity.sample.view.foucs.DefaultAutoFocusMarker
import com.trinity.sample.view.foucs.MarkerLayout
import kotlinx.android.synthetic.main.fragment_media.*
import java.io.File
import java.text.SimpleDateFormat
import java.util.*

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class RecordActivity : AppCompatActivity(), OnRecordingListener, OnRenderListener, RecordButton.OnGestureListener,
  SharedPreferences.OnSharedPreferenceChangeListener, CameraCallback {

  companion object {
    private const val SETTING_TAG = "setting_tag"
    private const val MUSIC_TAG = "music_tag"
    private const val MEDIA_TAG = "media_tag"
  }

  private lateinit var mRecord: TrinityRecord
  private lateinit var mLineView: LineProgressView
  private lateinit var mSwitchCamera: ImageView
  private lateinit var mFlash: ImageView
  private lateinit var mInsideBottomSheet: FrameLayout
  private lateinit var mMarkerLayout: MarkerLayout
  private val mFlashModes = arrayOf(Flash.TORCH, Flash.OFF, Flash.AUTO)
  private val mFlashImage = arrayOf(R.mipmap.ic_flash_on, R.mipmap.ic_flash_off, R.mipmap.ic_flash_auto)
  private var mFlashIndex = 0
  private val mMedias = mutableListOf<MediaItem>()
  private val mRecordDurations = mutableListOf<Int>()
  private var mCurrentRecordDuration = 0
  private var mHardwareEncode = false
  private var mFrame = Frame.FIT
  private var mRecordResolution = "720P"
  private var mFrameRate = 25
  private var mChannels = 1
  private var mSampleRate = 44100
  private var mVideoBitRate = 18432000
  private var mAudioBitRate = 12800
  private var mRecordDuration = 60 * 1000
  private var mAutoFocusMarker = DefaultAutoFocusMarker()
  private var mPermissionDenied = false

  @SuppressLint("ClickableViewAccessibility")
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    setContentView(R.layout.activity_record)

//    openLog()
    mMarkerLayout = findViewById(R.id.mark_layout)
    mMarkerLayout.onMarker(MarkerLayout.TYPE_AUTOFOCUS, mAutoFocusMarker)
    val preview = findViewById<TrinityPreviewView>(R.id.preview)
    mLineView = findViewById(R.id.line_view)

    mRecord = TrinityRecord(preview)
    mRecord.setOnRenderListener(this)
    mRecord.setOnRecordingListener(this)
    mRecord.setCameraCallback(this)
    val recordButton = findViewById<RecordButton>(R.id.record_button)
    recordButton.setOnGestureListener(this)
    mSwitchCamera = findViewById(R.id.switch_camera)
    switchCamera()
    mFlash = findViewById(R.id.flash)
    flash()
    val deleteView = findViewById<ImageView>(R.id.delete)
    deleteFile(deleteView)
    setRate()

    findViewById<View>(R.id.music)
      .setOnClickListener {
        showMusic()
      }

    mInsideBottomSheet = findViewById(R.id.frame_container)
    findViewById<View>(R.id.setting)
      .setOnClickListener {
        mInsideBottomSheet.visibility = View.VISIBLE
        showSetting()
      }
    setFrame()

    findViewById<View>(R.id.done)
      .setOnClickListener {
        if (mMedias.isNotEmpty()) {
          val intent = Intent(this, EditorActivity::class.java)
          intent.putExtra("medias", mMedias.toTypedArray())
          startActivity(intent)
        }
      }
    findViewById<View>(R.id.photo)
        .setOnClickListener {
          showMedia()
        }
    preview.setOnTouchListener { v, event ->
      mRecord.focus(PointF(event.x, event.y))
      true
    }
  }

  override fun dispatchOnFocusStart(where: PointF) {
    runOnUiThread {
      mMarkerLayout.onEvent(MarkerLayout.TYPE_AUTOFOCUS, arrayOf(where))
      mAutoFocusMarker.onAutoFocusStart(AutoFocusTrigger.GESTURE, where)
    }
  }

  override fun dispatchOnFocusEnd(success: Boolean, where: PointF) {
    runOnUiThread {
      mAutoFocusMarker.onAutoFocusEnd(AutoFocusTrigger.GESTURE, success, where)
    }
  }

  private fun setFrame() {
    mRecord.setFrame(mFrame)
  }

  fun onMediaResult(medias: MutableList<MediaItem>) {
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    if (behavior.state != BottomSheetBehavior.STATE_EXPANDED) {
      behavior.state = BottomSheetBehavior.STATE_EXPANDED
    } else {
      behavior.state = BottomSheetBehavior.STATE_HIDDEN
    }
    medias.forEach {
      mRecordDurations.add(it.duration)
      mLineView.addProgress(it.duration * 1.0f / mRecordDuration)
      mMedias.add(it)
    }
  }

  private fun showMedia() {
    var mediaFragment = supportFragmentManager.findFragmentByTag(MEDIA_TAG)
    if (mediaFragment == null) {
      mediaFragment = MediaFragment()
      supportFragmentManager.transaction {
        replace(R.id.frame_container, mediaFragment, MEDIA_TAG)
      }
    }
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    if (behavior.state != BottomSheetBehavior.STATE_EXPANDED) {
      behavior.state = BottomSheetBehavior.STATE_EXPANDED
    } else {
      behavior.state = BottomSheetBehavior.STATE_HIDDEN
    }
  }

  private fun showMusic() {
    var musicFragment = supportFragmentManager.findFragmentByTag(MUSIC_TAG)
    if (musicFragment == null) {
      musicFragment = MusicFragment.newInstance()
      supportFragmentManager.transaction {
        replace(R.id.frame_container, musicFragment, MUSIC_TAG)
      }
    }
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    if (behavior.state != BottomSheetBehavior.STATE_EXPANDED) {
      behavior.state = BottomSheetBehavior.STATE_EXPANDED
    } else {
      behavior.state = BottomSheetBehavior.STATE_HIDDEN
    }
  }

  fun closeBottomSheet() {
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    behavior.state = BottomSheetBehavior.STATE_HIDDEN
  }

  private fun showSetting() {
    var settingFragment = supportFragmentManager.findFragmentByTag(SETTING_TAG)
    if (settingFragment == null) {
      settingFragment = SettingFragment.newInstance()
      supportFragmentManager.transaction {
        replace(R.id.frame_container, settingFragment, SETTING_TAG)
      }
    }
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    if (behavior.state != BottomSheetBehavior.STATE_EXPANDED) {
      behavior.state = BottomSheetBehavior.STATE_EXPANDED
    } else {
      behavior.state = BottomSheetBehavior.STATE_HIDDEN
    }
  }

  private fun setRate() {
    val group = findViewById<RadioGroup>(R.id.rate_bar)
    group.setOnCheckedChangeListener { _, checkedId ->
      when (checkedId) {
        R.id.rate_quarter -> mRecord.setSpeed(Speed.VERY_SLOW)
        R.id.rate_half -> mRecord.setSpeed(Speed.SLOW)
        R.id.rate_origin -> mRecord.setSpeed(Speed.NORMAL)
        R.id.rate_double -> mRecord.setSpeed(Speed.FAST)
        R.id.rate_double_power2 -> mRecord.setSpeed(Speed.VERY_FAST)
      }
    }
  }

  private fun switchCamera() {
    mSwitchCamera.setOnClickListener {
      mRecord.switchCamera()
    }
  }

  private fun flash() {
    mFlash.setOnClickListener {
      mRecord.flash(mFlashModes[mFlashIndex % mFlashModes.size])
      mFlash.setImageResource(mFlashImage[mFlashIndex % mFlashImage.size])
      mFlashIndex++
    }
  }

  private fun deleteFile(view: View) {
    view.setOnClickListener {
      if (mMedias.isNotEmpty()) {
        mMedias.removeAt(mMedias.size - 1)
//        val file = File(media.path)
//        if (file.exists()) {
//          file.delete()
//        }
        mLineView.deleteProgress()
      }
      if (mRecordDurations.isNotEmpty()) {
        mRecordDurations.removeAt(mRecordDurations.size - 1)
      }
    }
  }

  override fun onDown() {
    var duration = 0
    mRecordDurations.forEach {
      duration += it
    }
    Log.i("trinity", "duration: $duration size: ${mRecordDurations.size}")
    if (duration >= mRecordDuration) {
      Toast.makeText(this, "已达最大时长", Toast.LENGTH_LONG).show()
      return
    }
    val date = SimpleDateFormat("yyyyMMdd_HHmmss").format(Date())
    val path = externalCacheDir?.absolutePath + "/VID_$date.mp4"
    var width = 720
    var height = 1280
    when (mRecordResolution) {
      "1080P" -> {
        width = 1080
        height = 1920
      }
      "720P" -> {
        width = 720
        height = 1280
      }
      "540P" -> {
        width = 544
        height = 960
      }
      "480P" -> {
        width = 480
        height = 848
      }
      "360P" -> {
        width = 368
        height = 640
      }
    }
    mRecord.startRecording(path, width, height,
      mVideoBitRate, mFrameRate, mHardwareEncode,
      mSampleRate, mChannels, mAudioBitRate, mRecordDuration)
    val media = MediaItem(path, "video", width, height)
    mMedias.add(media)
  }

  override fun onUp() {
    mRecord.stopRecording()
    mRecordDurations.add(mCurrentRecordDuration)
    val item = mMedias[mMedias.size - 1]
    item.duration = mCurrentRecordDuration
    runOnUiThread {
      mLineView.addProgress(mCurrentRecordDuration * 1.0f / mRecordDuration)
    }
  }

  fun setMusic(music: String) {
    val musicInfo = MusicInfo(music)
    mRecord.setBackgroundMusic(musicInfo)
    closeBottomSheet()
  }

  override fun onClick() {
  }


//  private fun openLog() {
//    val logPath = Environment.getExternalStorageDirectory().absolutePath + "/trinity"
//    if (BuildConfig.DEBUG) {
//      Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
//      Xlog.setConsoleLogOpen(true)
//    } else {
//      Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
//      Xlog.setConsoleLogOpen(false)
//    }
//    Log.setLogImp(Xlog())
//  }
//
//  private fun closeLog() {
//    Log.appenderClose()
//  }

  override fun onRecording(currentTime: Int, duration: Int) {
    mCurrentRecordDuration = currentTime
    runOnUiThread {
      mLineView.setLoadingProgress(currentTime * 1.0f / duration)
    }
  }

  override fun onResume() {
    super.onResume()
    val preferences = PreferenceManager.getDefaultSharedPreferences(this)
    preferences.registerOnSharedPreferenceChangeListener(this)
    mHardwareEncode = !preferences.getBoolean("soft_encode", false)
    val renderType = preferences.getString("preview_render_type", "CROP")
    mFrame = if (renderType == "FIT") {
      Frame.FIT
    } else {
      Frame.CROP
    }
    setFrame()

    mRecordResolution = preferences.getString("record_resolution", "720P") ?: "720P"
    try {
      mFrameRate = (preferences.getString("frame_rate", "25") ?: "25").toInt()
      mChannels = (preferences.getString("channels", "1") ?: "1").toInt()
      mSampleRate = (preferences.getString("sample_rate", "44100") ?: "44100").toInt()
      mVideoBitRate = (preferences.getString("video_bit_rate", "18432") ?: "18432").toInt()
      mAudioBitRate = (preferences.getString("audio_bit_rate", "128") ?: "128").toInt()
      mRecordDuration = (preferences.getString("record_duration", "60000") ?: "60000").toInt()
    } catch (e: Exception) {
      e.printStackTrace()
    }
    setPreviewResolution(preferences.getString("preview_resolution", "720P"))
//    mLineView.setMaxDuration(mRecordDuration)
  }

  override fun onPause() {
    super.onPause()
    mPermissionDenied = false
    mRecord.stopPreview()
    println("onPause")
    PreferenceManager.getDefaultSharedPreferences(this).unregisterOnSharedPreferenceChangeListener(this)
  }

  override fun onDestroy() {
    super.onDestroy()
    println("onDestroy")
//    closeLog()
  }

  override fun onSurfaceCreated() {
    println("onSurfaceCreated")
  }

  override fun onDrawFrame(textureId: Int, width: Int, height: Int, matrix: FloatArray?): Int {
    return 0
  }

  override fun onSurfaceDestroy() {
    println("onSurfaceDestroy")
  }

  override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String) {
    when (key) {
      "soft_encode" -> mHardwareEncode = !sharedPreferences.getBoolean(key, false)
      "preview_render_type" -> {
        val type = sharedPreferences.getString(key, "CROP")
        mFrame = if (type == "FIT") {
          Frame.FIT
        } else {
          Frame.CROP
        }
        setFrame()
      }
      "preview_resolution" -> {
        val resolution = sharedPreferences.getString("preview_resolution", "720P")
        setPreviewResolution(resolution)
      }
      "record_resolution" -> {
        mRecordResolution = sharedPreferences.getString(key, "720P") ?: "720P"
      }
      "frame_rate" -> {
        val frameRate = sharedPreferences.getString(key, "25") ?: "25"
        mFrameRate = frameRate.toInt()
      }
      "video_bit_rate" -> {
        val videoBitRate = sharedPreferences.getString(key, "18432000") ?: "18432000"
        mVideoBitRate = videoBitRate.toInt()
      }
      "channels" -> {
        val channels = sharedPreferences.getString(key, "1") ?: "1"
        mChannels = channels.toInt()
      }
      "sample_rate" -> {
        val sampleRate = sharedPreferences.getString(key, "44100") ?: "44100"
        mSampleRate = sampleRate.toInt()
      }
      "audio_bit_rate" -> {
        val audioBitRate = sharedPreferences.getString(key, "128000") ?: "128000"
        mAudioBitRate = audioBitRate.toInt()
      }
      "record_duration" -> {
        val recordDuration = sharedPreferences.getString(key, "60000") ?: "60000"
        mRecordDuration = recordDuration.toInt()
      }
    }
  }

  private fun setPreviewResolution(resolution: String?) {
    if (mPermissionDenied) {
      return
    }
    mRecord.stopPreview()
    var previewResolution = PreviewResolution.RESOLUTION_1280x720
    if (resolution == "1080P") {
      previewResolution = PreviewResolution.RESOLUTION_1920x1080
    } else if (resolution == "720P") {
      previewResolution = PreviewResolution.RESOLUTION_1280x720
    }

    askPermission(Manifest.permission.CAMERA,
      Manifest.permission.RECORD_AUDIO,
      Manifest.permission.WRITE_EXTERNAL_STORAGE,
      Manifest.permission.READ_EXTERNAL_STORAGE) {
      mRecord.startPreview(previewResolution)
    }.onDeclined {
      if (it.hasDenied()) {
        mPermissionDenied = true
        AlertDialog.Builder(this)
          .setMessage("请允许请求的所有权限")
          .setPositiveButton("请求") { _, _->
            it.askAgain()
          }.setNegativeButton("拒绝") { dialog, _->
            dialog.dismiss()
            finish()
          }.show()
      }
      if (it.hasForeverDenied()) {
        it.goToSettings()
      }
    }
  }

  override fun onBackPressed() {
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    if (behavior.state == BottomSheetBehavior.STATE_EXPANDED) {
      closeBottomSheet()
    } else {
      super.onBackPressed()
    }
  }
}