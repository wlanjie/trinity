package com.trinity.sample

import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.os.Environment
import android.view.View
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.RadioGroup
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.transaction
import androidx.preference.PreferenceManager
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.tencent.bugly.crashreport.CrashReport
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import com.trinity.OnRecordingListener
import com.trinity.camera.Flash
import com.trinity.camera.TrinityPreviewView
import com.trinity.core.MusicInfo
import com.trinity.core.RenderType
import com.trinity.listener.OnRenderListener
import com.trinity.record.PreviewResolution
import com.trinity.record.Speed
import com.trinity.record.TrinityRecord
import com.trinity.record.VideoConfiguration
import com.trinity.sample.fragment.MusicFragment
import com.trinity.sample.fragment.SettingFragment
import com.trinity.sample.view.LineProgressView
import com.trinity.sample.view.RecordButton
import java.io.File
import java.text.SimpleDateFormat
import java.util.*

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class RecordActivity : AppCompatActivity(), OnRecordingListener, OnRenderListener, RecordButton.OnGestureListener,
  SharedPreferences.OnSharedPreferenceChangeListener {

  companion object {
    private const val SETTING_TAG = "setting_tag"
    private const val MUSIC_TAG = "music_tag"
  }

  private lateinit var mRecord: TrinityRecord
  private lateinit var mLineView: LineProgressView
  private lateinit var mSwitchCamera: ImageView
  private lateinit var mFlash: ImageView
  private lateinit var mInsideBottomSheet: FrameLayout
  private val mFlashModes = arrayOf(Flash.TORCH, Flash.OFF, Flash.AUTO)
  private val mFlashImage = arrayOf(R.mipmap.ic_flash_on, R.mipmap.ic_flash_off, R.mipmap.ic_flash_auto)
  private var mFlashIndex = 0
  private val mRecordFiles = mutableListOf<String>()
  private val mRecordDurations = mutableListOf<Int>()
  private var mCurrentRecordDuration = 0
  private var mHardwareEncode = false
  private var mRenderType = RenderType.CROP
  private var mRecordResolution = "720P"
  private var mFrameRate = 25
  private var mChannels = 1
  private var mSampleRate = 44100
  private var mVideoBitRate = 18432000
  private var mAudioBitRate = 12800
  private var mRecordDuration = 60 * 1000

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    setContentView(R.layout.activity_record)

    openLog()
    val preview = findViewById<TrinityPreviewView>(R.id.preview)
    val videoConfiguration = VideoConfiguration()
    mLineView = findViewById(R.id.line_view)

    mRecord = TrinityRecord(preview, videoConfiguration)
    mRecord.setOnRenderListener(this)
    mRecord.setOnRecordingListener(this)
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
    setRenderType()

    findViewById<View>(R.id.done)
      .setOnClickListener {
        if (mRecordFiles.isNotEmpty()) {
          startActivity(Intent(this, EditorActivity::class.java))
        }
      }
  }

  private fun setRenderType() {
    mRecord.setRenderType(mRenderType)
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
      if (mRecordFiles.isNotEmpty()) {
        val path = mRecordFiles.removeAt(mRecordFiles.size - 1)
        val file = File(path)
        if (file.exists()) {
          file.delete()
        }
        mLineView.remove()
      }
    }
    if (mRecordDurations.isNotEmpty()) {
      mRecordDurations.removeAt(mRecordDurations.size - 1)
    }
  }

  override fun onDown() {
    var duration = 0
    mRecordDurations.forEach {
      duration += it
    }
    if (duration >= mRecordDuration) {
      Toast.makeText(this, "已达最大时长", Toast.LENGTH_LONG).show()
      return
    }
    val date = SimpleDateFormat("yyyyMMdd_HHmmss").format(Date())
    val path = externalCacheDir?.absolutePath + "/VID_$date.mp4"
    mRecordFiles.add(path)
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
    mRecord.startEncode(path, width, height,
      mVideoBitRate, mFrameRate, mHardwareEncode,
      mSampleRate, mChannels, mAudioBitRate, mRecordDuration)
  }

  override fun onUp() {
    mRecord.stopEncode()
    mRecordDurations.add(mCurrentRecordDuration)
    runOnUiThread {
      mLineView.stop()
    }
  }

  fun setMusic(music: String) {
    val musicInfo = MusicInfo(music)
    mRecord.setBackgroundMusic(musicInfo)
  }

  override fun onClick() {
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
    mCurrentRecordDuration = currentTime
    runOnUiThread {
      mLineView.setDuration(currentTime)
    }
  }

  override fun onResume() {
    super.onResume()
    val preferences = PreferenceManager.getDefaultSharedPreferences(this)
    preferences.registerOnSharedPreferenceChangeListener(this)
    mHardwareEncode = !preferences.getBoolean("soft_encode", false)
    val renderType = preferences.getString("preview_render_type", "CROP")
    mRenderType = if (renderType == "FIX_XY") {
      RenderType.FIX_XY
    } else {
      RenderType.CROP
    }
    setRenderType()

    mRecordResolution = preferences.getString("record_resolution", "720P") ?: "720P"
    try {
      mFrameRate = (preferences.getString("frame_rate", "25") ?: "25").toInt()
      mChannels = (preferences.getString("channels", "1") ?: "1").toInt()
      mSampleRate = (preferences.getString("sample_rate", "44100") ?: "44100").toInt()
      mVideoBitRate = (preferences.getString("video_bit_rate", "18432000") ?: "18432000").toInt()
      mAudioBitRate = (preferences.getString("audio_bit_rate", "128000") ?: "128000").toInt()
      mRecordDuration = (preferences.getString("record_duration", "60000") ?: "60000").toInt()
    } catch (e: Exception) {
      e.printStackTrace()
    }
    setPreviewResolution(preferences.getString("preview_resolution", "720P"))
    mLineView.setMaxDuration(mRecordDuration)
  }

  override fun onPause() {
    super.onPause()
    mRecord.stopPreview()
    PreferenceManager.getDefaultSharedPreferences(this).unregisterOnSharedPreferenceChangeListener(this)
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

  override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String) {
    when (key) {
      "soft_encode" -> mHardwareEncode = !sharedPreferences.getBoolean(key, false)
      "preview_render_type" -> {
        val type = sharedPreferences.getString(key, "CROP")
        mRenderType = if (type == "FIX_XY") {
          RenderType.FIX_XY
        } else {
          RenderType.CROP
        }
        setRenderType()
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
    mRecord.stopPreview()
    var previewResolution = PreviewResolution.RESOLUTION_1280x720
    if (resolution == "1080P") {
      previewResolution = PreviewResolution.RESOLUTION_1920x1080
    } else if (resolution == "720P") {
      previewResolution = PreviewResolution.RESOLUTION_1280x720
    }
    mRecord.startPreview(previewResolution)
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