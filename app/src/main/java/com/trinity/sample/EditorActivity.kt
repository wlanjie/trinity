package com.trinity.sample

import android.annotation.SuppressLint
import android.content.Intent
import android.graphics.BitmapFactory
import android.graphics.Point
import android.os.Bundle
import android.util.TypedValue
import android.view.*
import android.view.GestureDetector.SimpleOnGestureListener
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.coordinatorlayout.widget.CoordinatorLayout
import androidx.fragment.app.transaction
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.gson.JsonObject
import com.trinity.core.TrinityCore
import com.trinity.editor.EffectType
import com.trinity.editor.MediaClip
import com.trinity.editor.TimeRange
import com.trinity.editor.TrinityVideoEditor
import com.trinity.sample.editor.*
import com.trinity.sample.entity.Effect
import com.trinity.sample.entity.EffectInfo
import com.trinity.sample.entity.Filter
import com.trinity.sample.entity.MediaItem
import com.trinity.sample.fragment.MusicFragment
import com.trinity.sample.listener.OnEffectTouchListener
import com.trinity.sample.view.*
import org.json.JSONObject
import java.nio.charset.Charset
import java.util.*

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class EditorActivity : AppCompatActivity(), ViewOperator.AnimatorListener, TabLayout.BaseOnTabSelectedListener<TabLayout.Tab>, ThumbLineBar.OnBarSeekListener, PlayerListener, OnEffectTouchListener {
  companion object {
    private const val MUSIC_TAG = "music"
    private const val USE_ANIMATION_REMAIN_TIME = 300 * 1000
  }

  private lateinit var mSurfaceContainer: FrameLayout
  private lateinit var mSurfaceView: SurfaceView
  private lateinit var mPasterContainer: FrameLayout
  private lateinit var mTabLayout: TabLayout
  private lateinit var mBottomLinear: HorizontalScrollView
  private lateinit var mPlayImage: ImageView
  private lateinit var mPauseImage: ImageView
  private lateinit var mViewOperator: ViewOperator
  private lateinit var mActionBar: RelativeLayout
  private lateinit var mThumbLineBar: OverlayThumbLineBar
  private lateinit var mInsideBottomSheet: FrameLayout
  private lateinit var mBottomSheetLayout: CoordinatorLayout
  private lateinit var mVideoEditor: TrinityVideoEditor
  private var mLutFilter: LutFilterChooser ?= null
  private var mEffect: EffectChooser ?= null
  private var mUseInvert = false
  private var mCanAddAnimation = true
  private var mThumbLineOverlayView: ThumbLineOverlay.ThumbLineOverlayView ?= null
  private var mThumbnailFetcher: ThumbnailFetcher ?= null
  private lateinit var mEffectController: EffectController
//  private val mEffects = LinkedHashMap<String, EffectInfo>()
  private val mEffects =  LinkedList<EffectInfo>()
  private var mStartTime: Long = 0
  private val mActionIds = mutableMapOf<String, Int>()
  private var mFilterId = -1
  private var mFlashWhiteId = -1
  private var mSplitScreenTwoId = -1
  private var mSplitScreenThreeId = -1
  private var mSplitScreenFourId = -1
  private var mSplitScreenSixId = -1
  private var mSplitScreenNineId = -1
  private var mBlurSplitScreenId = -1
  private var mMusicId = -1

  @SuppressLint("ClickableViewAccessibility")
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_editor)
    mVideoEditor = TrinityCore.createEditor(this)
//    openLog()

    mEffectController = EffectController(this, mVideoEditor)
    addTabLayout()
    mSurfaceContainer = findViewById(R.id.surface_container)
    mSurfaceView = findViewById(R.id.surface_view)
    mPasterContainer = findViewById(R.id.paster_view)
    val gesture = GestureDetector(this, mOnGestureListener)
    mPasterContainer.setOnTouchListener { _, event -> gesture.onTouchEvent(event) }
    mBottomLinear = findViewById(R.id.editor_bottom_tab)
    mPlayImage = findViewById(R.id.play)
    mPauseImage = findViewById(R.id.pause)
    mActionBar = findViewById(R.id.action_bar)
    mActionBar.setBackgroundDrawable(null)
    mThumbLineBar = findViewById(R.id.thumb_line_bar)
    initThumbLineBar()

    val rootView = findViewById<RelativeLayout>(R.id.root_view)
    mViewOperator = ViewOperator(rootView, mActionBar, mSurfaceView, mTabLayout, mPasterContainer, mPlayImage)
    mViewOperator.setAnimatorListener(this)
    mInsideBottomSheet = findViewById(R.id.frame_container)
    mBottomSheetLayout = findViewById(R.id.editor_coordinator)

    mPlayImage.setOnClickListener { mVideoEditor.resume() }
    mPauseImage.setOnClickListener { mVideoEditor.pause() }
    rootView.setOnClickListener {
      mViewOperator.hideBottomEditorView(EditorPage.FILTER)
    }

    mVideoEditor.setSurfaceView(mSurfaceView)
    val medias = intent.getSerializableExtra("medias") as Array<MediaItem>
//    val clip = MediaClip(file.absolutePath, TimeRange(0, 10000))
//    mVideoEditor.insertClip(clip)
    medias.forEach {
      val clip = MediaClip(it.path, TimeRange(0, it.duration.toLong()))
      mVideoEditor.insertClip(clip)
    }
    val result = mVideoEditor.play(true)
    if (result != 0) {
      Toast.makeText(this, "播放失败: $result", Toast.LENGTH_SHORT).show()
    }

    findViewById<View>(R.id.next)
        .setOnClickListener {
          startActivity(Intent(this, VideoExportActivity::class.java))
        }
  }

  private fun initThumbLineBar() {
    val thumbnailSize = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 32f, resources.displayMetrics).toInt()
    val thumbnailPoint = Point(thumbnailSize, thumbnailSize)

    if (mThumbnailFetcher == null) {
      mThumbnailFetcher = object: ThumbnailFetcher {
        override fun addVideoSource(path: String) {
        }

        override fun requestThumbnailImage(times: LongArray, l: ThumbnailFetcher.OnThumbnailCompletionListener) {
          val bitmap = BitmapFactory.decodeResource(resources, R.drawable.ic_launcher)
          l.onThumbnail(bitmap, 0)
        }

        override fun getTotalDuration(): Long {
          return mVideoEditor.getVideoDuration()
        }

        override fun release() {
        }
      }
    } else {
      mThumbnailFetcher?.release()
      mThumbnailFetcher?.addVideoSource("/sdcard/test.mp4")
    }

    val config = ThumbLineConfig.Builder()
        .thumbnailFetcher(mThumbnailFetcher)
        .screenWidth(windowManager.defaultDisplay.width)
        .thumbPoint(thumbnailPoint)
        .thumbnailCount(10)
        .build()
    mThumbLineOverlayView = object: ThumbLineOverlay.ThumbLineOverlayView {
      val rootView = LayoutInflater.from(applicationContext).inflate(R.layout.timeline_overlay, null)

      override val container: ViewGroup
        get() = rootView as ViewGroup
      override val headView: View
        get() = rootView.findViewById(R.id.head_view)
      override val tailView: View
        get() = rootView.findViewById(R.id.tail_view)
      override val middleView: View
        get() = rootView.findViewById(R.id.middle_view)

    }
    mThumbLineBar.setup(config, this, this)
    mEffectController.setThumbLineBar(mThumbLineBar)
  }

  private fun changePlayResource() {

  }

  override fun onThumbLineBarSeek(duration: Long) {
//    mVideoEditor.seek(duration)
    mThumbLineBar.pause()
    changePlayResource()
    mCanAddAnimation = if (mUseInvert) {
      duration > USE_ANIMATION_REMAIN_TIME
    } else {
      mVideoEditor.getVideoDuration() - duration < USE_ANIMATION_REMAIN_TIME
    }
  }

  override fun onThumbLineBarSeekFinish(duration: Long) {
//    mVideoEditor.seek(duration)
    mThumbLineBar.pause()
    changePlayResource()
    mCanAddAnimation = if (mUseInvert) {
      duration > USE_ANIMATION_REMAIN_TIME
    } else {
      mVideoEditor.getVideoDuration() - duration < USE_ANIMATION_REMAIN_TIME
    }
  }

  override fun getCurrentDuration(): Long {
    return mVideoEditor.getCurrentPosition()
  }

  override fun getDuration(): Long {
    return mVideoEditor.getVideoDuration()
  }

  override fun updateDuration(duration: Long) {
  }

  private fun addTabLayout() {
    mTabLayout = findViewById(R.id.tab_layout)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.filter)
        .setIcon(R.mipmap.ic_filter), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.effect)
        .setIcon(R.mipmap.ic_effect), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.music)
        .setIcon(R.mipmap.ic_music), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.mv)
        .setIcon(R.mipmap.ic_mv), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.speed_time)
        .setIcon(R.mipmap.ic_speed_time), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.gif)
        .setIcon(R.mipmap.ic_gif), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.subtitle)
        .setIcon(R.mipmap.ic_subtitle), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.transition)
        .setIcon(R.mipmap.ic_transition), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.paint)
        .setIcon(R.mipmap.ic_paint), false)
    mTabLayout.addTab(mTabLayout.newTab().setText(R.string.cover)
        .setIcon(R.mipmap.ic_cover), false)
    mTabLayout.addOnTabSelectedListener(this)
  }

  override fun onTabReselected(tab: TabLayout.Tab) {
  }

  override fun onTabUnselected(tab: TabLayout.Tab) {
  }

  override fun onTabSelected(tab: TabLayout.Tab) {
    when (tab.text) {
      getString(R.string.filter) -> {
        setActiveIndex(EditorPage.FILTER)
      }
      getString(R.string.effect) -> {
        setActiveIndex(EditorPage.FILTER_EFFECT)
      }
      getString(R.string.music) -> {
        setActiveIndex(EditorPage.AUDIO_MIX)
      }
    }
  }

  private fun setActiveIndex(page: EditorPage) {
    mBottomSheetLayout.visibility = View.GONE
    when (page) {
      EditorPage.FILTER -> {
        if (mLutFilter == null) {
          mLutFilter = LutFilterChooser(this) {
            bottomEvent(it)
            mTabLayout.getTabAt(mTabLayout.selectedTabPosition)?.customView?.isSelected = false
          }
        }
        mLutFilter?.let {
          mViewOperator.showBottomView(it)
        }
      }
      EditorPage.FILTER_EFFECT -> {
        if (mEffect == null) {
          mEffect = EffectChooser(this)
          mEffect?.setOnEffectTouchListener(this)
        }
        mEffect?.addThumbView(mThumbLineBar)
        mEffect?.let {
          mViewOperator.showBottomView(it)
        }
      }
      EditorPage.AUDIO_MIX -> {
        showMusic()
      }
    }
  }

  override fun onEffectTouchEvent(event: Int, effect: Effect) {
    if (event == MotionEvent.ACTION_DOWN) {
      mStartTime = mVideoEditor.getCurrentPosition()
      mVideoEditor.resume()
      if (effect.id == EffectId.UNDO.ordinal) {
        return
      } else {
        val stream = assets.open(effect.effect)
        val bytes = ByteArray(stream.available())
        stream.read(bytes)
        val effectJson = String(bytes, Charset.forName("utf-8"))
        val actionId = mVideoEditor.addAction(effectJson)
        mActionIds[effect.name] = actionId
      }
      effect.startTime = mStartTime.toInt()
      mEffectController.onEventAnimationFilterLongClick(effect)
    } else if (event == MotionEvent.ACTION_UP) {
      mVideoEditor.pause()
      if (effect.id == EffectId.UNDO.ordinal) {
        if (!mEffects.isEmpty()) {
          val info = mEffects.removeLast()
          mEffectController.onEventAnimationFilterDelete(Effect(0, "", "", ""))
          mVideoEditor.deleteAction(info.actionId)
          mVideoEditor.seek(info.startTime.toInt())
        }
        return
      } else {
        val stream = assets.open(effect.effect)
        val bytes = ByteArray(stream.available())
        stream.read(bytes)
        val effectJson = JSONObject(String(bytes, Charset.forName("utf-8")))
        val effectItemArrayJson = effectJson.optJSONArray("effect") ?: return
        for (i in 0 until effectItemArrayJson.length()) {
          val effectItemJson = effectItemArrayJson.optJSONObject(i)
          effectItemJson.put("endTime", mVideoEditor.getCurrentPosition())
          effectItemJson.put("startTime", mStartTime)
        }
        val effectInfo = EffectInfo()
        val actionId = mActionIds[effect.name] ?: return
        effectInfo.actionId = actionId
        effectInfo.startTime = mStartTime
        mEffects.add(effectInfo)
        mVideoEditor.updateAction(effectJson.toString(), actionId)
        mEffectController.onEventAnimationFilterClickUp(effect)
      }
    }
  }

  private fun bottomEvent(param: Any) {
    if (param is Filter) {
      setFilter(param)
    }
  }

  override fun onShowAnimationEnd() {
    mVideoEditor.pause()
  }

  override fun onHideAnimationEnd() {
    mVideoEditor.resume()
  }

  fun setMusic(path: String) {
    val jsonObject = JSONObject()
    jsonObject.put("path", path)
    if (mMusicId != -1) {
      mVideoEditor.updateMusic(jsonObject.toString(), mMusicId)
    } else {
      mVideoEditor.addMusic(jsonObject.toString())
    }
    closeBottomSheet()
  }

  fun closeBottomSheet() {
    val behavior = BottomSheetBehavior.from(mInsideBottomSheet)
    behavior.state = BottomSheetBehavior.STATE_HIDDEN
  }

  private fun showMusic() {
    mBottomSheetLayout.visibility = View.VISIBLE
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

  private fun setFilter(filter: Filter) {
    val jsonObject = JSONObject()
    jsonObject.put("effectType", EffectType.Filter.name)
    jsonObject.put("startTime", 0)
    jsonObject.put("endTime", Int.MAX_VALUE)
    jsonObject.put("lut", externalCacheDir?.absolutePath + "/filter/${filter.lut}")
    jsonObject.put("intensity", 1.0f)
    if (mFilterId == -1) {
      mFilterId = mVideoEditor.addAction(jsonObject.toString())
    } else {
      mVideoEditor.updateAction(jsonObject.toString(), mFilterId)
    }
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
//    closeLog()
  }

//  private fun openLog() {
//    val logPath = Environment.getExternalStorageDirectory().absolutePath + "/trinity"
//    Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
//    Xlog.setConsoleLogOpen(true)
//    Log.setLogImp(Xlog())
//  }
//
//  private fun closeLog() {
//    Log.appenderClose()
//  }

  private val mOnGestureListener = object: SimpleOnGestureListener() {

    override fun onSingleTapConfirmed(e: MotionEvent?): Boolean {
      return super.onSingleTapConfirmed(e)
    }
  }

}
