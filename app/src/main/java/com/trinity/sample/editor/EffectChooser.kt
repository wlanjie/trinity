package com.trinity.sample.editor

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.MotionEvent
import android.widget.FrameLayout
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.trinity.sample.R
import com.trinity.sample.adapter.EffectAdapter
import com.trinity.sample.entity.Effect
import com.trinity.sample.entity.EffectInfo
import com.trinity.sample.listener.OnEffectTouchListener
import java.nio.charset.Charset

/**
 * Created by wlanjie on 2019-07-26
 */
class EffectChooser : Chooser, EffectAdapter.OnItemTouchListener {

  private lateinit var mRecyclerView: RecyclerView
  private lateinit var mThumbnailContainer: FrameLayout
  private var mOnEffectTouchListener: OnEffectTouchListener ?= null

  constructor(context: Context) : super(context)

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs)

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr)

  override fun init() {
    LayoutInflater.from(context).inflate(R.layout.effect_view, this)
    mRecyclerView = findViewById(R.id.effect_recycler_view)
    mThumbnailContainer = findViewById(R.id.thumbnail_container)
    val stream = context.assets.open("effect.json")
    val size = stream.available()
    val buffer = ByteArray(size)
    stream.read(buffer)
    stream.close()
    val effects = Gson().fromJson<List<Effect>>(String(buffer, Charset.forName("UTF-8")), object: TypeToken<List<Effect>>(){}.type)
    val adapter = EffectAdapter(context, effects)
    adapter.setOnTouchListener(this)
    mRecyclerView.adapter = adapter
    mRecyclerView.layoutManager = LinearLayoutManager(context, RecyclerView.HORIZONTAL, false)
  }

  override fun isPlayerNeedZoom(): Boolean {
    return true
  }

  override fun getThumbContainer(): FrameLayout? {
    return mThumbnailContainer
  }

  override fun onTouchEvent(event: Int, position: Int, effect: String) {
    if (event == MotionEvent.ACTION_UP) {
      if (mThumbLineBar?.isTouching == false) {
        isClickable = true
        mOnEffectTouchListener?.onEffectTouchEvent(event, effect)
      }
    } else {
      if (mThumbLineBar?.isTouching == false) {
        isClickable = false
        mOnEffectTouchListener?.onEffectTouchEvent(event, effect)
      }
    }
  }

  fun setOnEffectTouchListener(l: OnEffectTouchListener) {
    mOnEffectTouchListener = l
  }
}