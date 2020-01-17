package com.trinity.sample.editor

import android.content.Context
import android.view.LayoutInflater
import android.view.MotionEvent
import android.widget.FrameLayout
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentPagerAdapter
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager.widget.ViewPager
import com.google.android.material.tabs.TabLayout
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.trinity.sample.EditorActivity
import com.trinity.sample.R
import com.trinity.sample.adapter.EffectAdapter
import com.trinity.sample.entity.Effect
import com.trinity.sample.fragment.EffectFilterFragment
import com.trinity.sample.listener.OnEffectTouchListener
import java.nio.charset.Charset

/**
 * Created by wlanjie on 2019-07-26
 */
class EffectChooser(context: Context, fragmentManager: FragmentManager) : Chooser(context), EffectAdapter.OnItemTouchListener, TabLayout.OnTabSelectedListener {

  private lateinit var mRecyclerView: RecyclerView
  private lateinit var mThumbnailContainer: FrameLayout
  private var mOnEffectTouchListener: OnEffectTouchListener ?= null

  override fun init() {
    LayoutInflater.from(context).inflate(R.layout.effect_view, this)
//    mRecyclerView = findViewById(R.id.effect_recycler_view)
    mThumbnailContainer = findViewById(R.id.thumbnail_container)
//    val stream = context.assets.open("effect.json")
//    val size = stream.available()
//    val buffer = ByteArray(size)
//    stream.read(buffer)
//    stream.close()
//    val effects = Gson().fromJson<List<Effect>>(String(buffer, Charset.forName("UTF-8")), object: TypeToken<List<Effect>>(){}.type)
//    val adapter = EffectAdapter(context, effects)
//    adapter.setOnTouchListener(this)
//    mRecyclerView.adapter = adapter
//    mRecyclerView.layoutManager = LinearLayoutManager(context, RecyclerView.HORIZONTAL, false)

    val tabLayout = findViewById<TabLayout>(R.id.tab_layout)
    val viewPager = findViewById<ViewPager>(R.id.view_pager)
    val activity = context as EditorActivity
    viewPager.adapter = PagerAdapter(context, this, activity.supportFragmentManager)
    tabLayout.setupWithViewPager(viewPager)
//    tabLayout.addTab(tabLayout.newTab().setText(R.string.dream))
//    tabLayout.addTab(tabLayout.newTab().setText(R.string.filter))
//    tabLayout.addTab(tabLayout.newTab().setText(R.string.split_screen))
//    tabLayout.addTab(tabLayout.newTab().setText(R.string.transition))
//    tabLayout.addOnTabSelectedListener(this)
  }

  override fun isPlayerNeedZoom(): Boolean {
    return true
  }

  override fun getThumbContainer(): FrameLayout? {
    return mThumbnailContainer
  }

  override fun onTouchEvent(event: Int, position: Int, effect: Effect) {
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

  override fun onTabReselected(tab: TabLayout.Tab?) {
  }

  override fun onTabUnselected(tab: TabLayout.Tab?) {
  }

  override fun onTabSelected(tab: TabLayout.Tab?) {
  }

  class PagerAdapter(private val context: Context,
                     private val listener: EffectAdapter.OnItemTouchListener,
                     fragmentManager: FragmentManager) : FragmentPagerAdapter(fragmentManager) {
    private val mPageTitle = arrayOf(R.string.dream, R.string.filter, R.string.split_screen, R.string.transition)
    private val mContent = arrayOf("dream.json", "effect_filter.json", "split_screen.json", "transition.json")

    override fun getItem(position: Int): Fragment {
      val content = mContent[position]
      val fragment = EffectFilterFragment.newInstance(content)
      fragment.setOnItemTouchListener(listener)
      return fragment
    }

    override fun getCount(): Int {
        return mPageTitle.size
    }

    override fun getPageTitle(position: Int): CharSequence? {
      return context.getString(mPageTitle[position])
    }
  }
}