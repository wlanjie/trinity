package com.trinity.sample

import android.annotation.SuppressLint
import android.content.Intent
import android.graphics.BitmapFactory
import android.graphics.Point
import android.os.Bundle
import android.os.Environment
import android.util.TypedValue
import android.view.*
import android.view.GestureDetector.SimpleOnGestureListener
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.bottomsheet.BottomSheetDialog
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import com.trinity.editor.EffectType
import com.trinity.editor.MediaClip
import com.trinity.editor.TimeRange
import com.trinity.editor.VideoEditor
import com.trinity.sample.adapter.MusicAdapter
import com.trinity.sample.editor.*
import com.trinity.sample.entity.EffectInfo
import com.trinity.sample.entity.Filter
import com.trinity.sample.entity.MediaItem
import com.trinity.sample.listener.OnEffectTouchListener
import com.trinity.sample.view.*
import com.trinity.sample.view.ThumbLineBar
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.async
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.charset.Charset

/**
 * Create by wlanjie on 2019/4/13-下午3:14
 */
class EditorActivity : AppCompatActivity(), ViewOperator.AnimatorListener, TabLayout.BaseOnTabSelectedListener<TabLayout.Tab>, ThumbLineBar.OnBarSeekListener, PlayerListener, OnEffectTouchListener {
  companion object {
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
  private lateinit var mVideoEditor: VideoEditor
  private var mLutFilter: LutFilterChooser ?= null
  private var mEffect: EffectChooser ?= null
  private var mUseInvert = false
  private var mCanAddAnimation = true
  private var mThumbLineOverlayView: ThumbLineOverlay.ThumbLineOverlayView ?= null
  private var mThumbnailFetcher: ThumbnailFetcher ?= null
  private lateinit var mEffectController: EffectController
  private var mStartTime: Long = 0
  private var mFlashWhiteId = -1

  @SuppressLint("ClickableViewAccessibility")
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_editor)
    mVideoEditor = VideoEditor(this)
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

    mPlayImage.setOnClickListener { mVideoEditor.resume() }
    mPauseImage.setOnClickListener { mVideoEditor.pause() }
    rootView.setOnClickListener {
      mViewOperator.hideBottomEditorView(EditorPage.FILTER)
    }

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
          return 10000
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
    return 10000
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
    println("onTabReselected")
  }

  override fun onTabUnselected(tab: TabLayout.Tab) {
    println("onTabUnselected")
  }

  override fun onTabSelected(tab: TabLayout.Tab) {
    println("onTabSelected")
    when (tab.text) {
      getString(R.string.filter) -> {
        setActiveIndex(EditorPage.FILTER)
      }
      getString(R.string.effect) -> {
        setActiveIndex(EditorPage.FILTER_EFFECT)
      }
    }
  }

  private fun setActiveIndex(page: EditorPage) {
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
    }
  }

  override fun onEffectTouchEvent(event: Int, info: EffectInfo) {
    if (event == MotionEvent.ACTION_DOWN) {
      mStartTime = mVideoEditor.getCurrentPosition()
      mVideoEditor.resume()
      if ("闪白" == info.path) {
        mFlashWhiteId = mVideoEditor.addAction(EffectType.FLASH_WHITE, 0, Long.MAX_VALUE)
      }
      mEffectController.onEventAnimationFilterLongClick(info)
    } else if (event == MotionEvent.ACTION_UP) {
      mVideoEditor.pause()
      if ("闪白" == info.path) {
        mVideoEditor.addAction(EffectType.FLASH_WHITE, mStartTime, mVideoEditor.getCurrentPosition(), mFlashWhiteId)
      }
      mEffectController.onEventAnimationFilterClickUp(info)
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

  private fun showMusic() {
    val dialog = BottomSheetDialog(this)
    val view = LayoutInflater.from(this).inflate(R.layout.bottom_sheet_filter, null)
    val recyclerView = view as RecyclerView
    recyclerView.layoutManager = LinearLayoutManager(this, RecyclerView.VERTICAL, false)
    recyclerView.addItemDecoration(DividerItemDecoration(this, DividerItemDecoration.VERTICAL))
    recyclerView.adapter = MusicAdapter(this) {}
    dialog.setContentView(view)
    dialog.setCancelable(true)
    dialog.setCanceledOnTouchOutside(true)
    dialog.show()
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
    mVideoEditor.addFilter(externalCacheDir?.absolutePath + "/filter/${filter.lut}", 0, Long.MAX_VALUE)
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