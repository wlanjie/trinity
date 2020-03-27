package com.trinity.sample.adapter

import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentPagerAdapter
import com.trinity.sample.fragment.PictureFragment
import com.trinity.sample.fragment.VideoFragment

class MediaPagerAdapter(fm: FragmentManager) : FragmentPagerAdapter(fm, FragmentPagerAdapter.BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT) {

  private val mItems = mutableListOf<Fragment>()

  init {
    mItems.add(VideoFragment())
    mItems.add(PictureFragment())
  }

  override fun getItem(position: Int): Fragment {
    return mItems[position]
  }

  override fun getCount(): Int {
    return 2
  }

  override fun getPageTitle(position: Int): CharSequence? {
    return if (position == 0) {
      "视频"
    } else {
      "图片"
    }
  }
}