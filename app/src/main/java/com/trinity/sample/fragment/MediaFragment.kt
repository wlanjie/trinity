package com.trinity.sample.fragment

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.viewpager.widget.ViewPager
import com.google.android.material.tabs.TabLayout
import com.trinity.sample.R
import com.trinity.sample.RecordActivity
import com.trinity.sample.adapter.MediaPagerAdapter
import com.trinity.sample.entity.MediaItem

class MediaFragment : Fragment() {

  private var mAdapter: MediaPagerAdapter ?= null
  private lateinit var mTabLayout: TabLayout

  override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
    return inflater.inflate(R.layout.fragment_media, container, false)
  }

  override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
    super.onViewCreated(view, savedInstanceState)
    mTabLayout = view.findViewById(R.id.tab_layout)
    val viewPager = view.findViewById<ViewPager>(R.id.view_pager)
    activity?.let {
      mAdapter = MediaPagerAdapter(it.supportFragmentManager)
      viewPager.adapter = mAdapter
    }
    mTabLayout.setupWithViewPager(viewPager)

    view.findViewById<View>(R.id.done)
        .setOnClickListener {
          val items = getSelectMedias()
          items?.let {
            if (activity is RecordActivity) {
              (activity as RecordActivity).onMediaResult(it)
            }
          }
        }
  }

  private fun getSelectMedias(): MutableList<MediaItem>? {
    val position = mTabLayout.selectedTabPosition
    return when (val fragment = mAdapter?.getItem(position)) {
      is VideoFragment -> fragment.getSelectMedias()
      is PictureFragment -> fragment.getSelectMedias()
      else -> null
    }
  }

}