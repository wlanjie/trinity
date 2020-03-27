/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.trinity.sample.adapter

import android.content.Context
import android.view.View
import android.view.ViewGroup
import androidx.viewpager.widget.PagerAdapter
import java.util.*

class ColorViewPagerAdapter : PagerAdapter() {

  private val mTabHolder: MutableList<ViewHolder> = ArrayList()

  override fun getCount(): Int {
    return mTabHolder.size
  }

  override fun instantiateItem(container: ViewGroup, position: Int): Any {
    val holder = mTabHolder[position]
    container.addView(holder.mItemView)
    holder.onBindViewHolder()
    return holder.mItemView
  }

  override fun getPageTitle(position: Int): CharSequence? {
    return mTabHolder[position].mTitle
  }

  override fun destroyItem(container: ViewGroup, position: Int, `object`: Any) {
    if (`object` is View) {
      container.removeView(`object`)
    }
  }

  override fun isViewFromObject(view: View, `object`: Any): Boolean {
    return view === `object`
  }

  fun addViewHolder(holder: ViewHolder) {
    mTabHolder.add(holder)
  }

  abstract class ViewHolder(context: Context?, title: String) {

    val mItemView = onCreateView(context)
    var mTitle = title

    protected abstract fun onCreateView(context: Context?): View

    abstract fun onBindViewHolder()

  }
}