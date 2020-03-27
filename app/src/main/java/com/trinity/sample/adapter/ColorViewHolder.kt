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
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.GradientDrawable
import android.util.SparseArray
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.BaseAdapter
import android.widget.GridView
import android.widget.ImageView
import com.trinity.sample.R
import java.util.*

class ColorViewHolder(private val mContext: Context,
                      title: String,
                      private val mIsStroke: Boolean,
                      private val mInitColor: Int) : ColorViewPagerAdapter.ViewHolder(mContext, title) {
  private var mGridView: GridView? = null
  private var mItemClickListener: OnItemClickListener? = null
  private val mGradientDrawCache = SparseArray<GradientDrawable>()
  private val mColorDrawCache = SparseArray<ColorDrawable>()

  override fun onCreateView(context: Context?): View {
    val rootView: View = LayoutInflater.from(context).inflate(R.layout.color_tab_content, null, false)
    mGridView = rootView.findViewById<View>(R.id.grid_view) as GridView
    return rootView
  }

  override fun onBindViewHolder() {
    val colorAdapter = ColorAdapter()
    colorAdapter.setData(initColors(mIsStroke, mInitColor))
    mGridView?.adapter = colorAdapter
  }

  private inner class ColorAdapter : BaseAdapter() {

    private var list: MutableList<ColorItem> = ArrayList()

    fun setData(data: List<ColorItem>?) {
      if (data == null || data.isEmpty()) {
        return
      }
      list.addAll(data)
      notifyDataSetChanged()
    }

    override fun getCount(): Int {
      return list.size
    }

    override fun getItem(position: Int): ColorItem {
      return list[position]
    }

    override fun getItemId(position: Int): Long {
      return 0
    }

    override fun getView(position: Int, convertView: View?, parent: ViewGroup): View? {
      val localViewHolder: ColorItemViewMediator
      var itemView = convertView
      if (itemView == null) {
        localViewHolder = ColorItemViewMediator(parent)
        itemView = localViewHolder.view
      } else {
        localViewHolder = itemView.tag as ColorItemViewMediator
      }

      val item = getItem(position)
      localViewHolder.setData(item, position == 0)
      if (mGridView?.checkedItemPosition == position) {
        localViewHolder.setSelected(true)
      } else {
        localViewHolder.setSelected(false)
      }
      localViewHolder.setListener(View.OnClickListener { v ->
        val mediator = v.tag as ColorItemViewMediator
        mItemClickListener?.onItemClick(mediator.data)
        mGridView?.setItemChecked(position, true)
      })
      return itemView
    }
  }

  private inner class ColorItemViewMediator internal constructor(parent: ViewGroup) {
    val view = View.inflate(parent.context, R.layout.item_text_color, null)
    var data: ColorItem ?= null
    private val mIvColor: ImageView
    private val select: View

    init {
      mIvColor = view.findViewById(R.id.iv_color)
      select = view.findViewById(R.id.selected)
      view.tag = this
    }

    fun setListener(listener: View.OnClickListener?) {
      view.setOnClickListener(listener)
    }

    fun setSelected(selected: Boolean) {
      select.visibility = if (selected) View.VISIBLE else View.GONE
    }

    fun setData(item: ColorItem, isNone: Boolean) {
      data = item
      if (isNone) {
        mIvColor.setImageResource(R.mipmap.ic_font_color_none)
        return
      }
      if (item.isStroke) {
        var drawable = mGradientDrawCache[item.strokeColor]
        if (drawable == null) {
          drawable = GradientDrawable()
          drawable.setStroke(8, item.strokeColor)
          drawable.shape = GradientDrawable.RECTANGLE
          mGradientDrawCache.put(item.strokeColor, drawable)
        }
        mIvColor.setImageDrawable(drawable)
      } else {
        var drawable = mColorDrawCache[item.color]
        if (drawable == null) {
          drawable = ColorDrawable(item.color)
          drawable.setBounds(0, 0, mIvColor.measuredWidth, mIvColor.measuredHeight)
          mColorDrawCache.put(item.color, drawable)
        }
        mIvColor.setImageDrawable(drawable)
      }
    }
  }

  private fun initColors(stroke: Boolean, initColor: Int): List<ColorItem> {
    val list: MutableList<ColorItem> = ArrayList()
    val colors = mContext.resources.obtainTypedArray(R.array.text_edit_colors)
    for (i in 0..34) {
      val color = colors.getColor(i, Color.WHITE)
      val ci = ColorItem()
      ci.color = color
      ci.isStroke = stroke
      list.add(ci)
      if (stroke) {
        ci.strokeColor = color
      }
    }
    colors.recycle()
    //添加无颜色
    val colorItem = ColorItem()
    colorItem.color = Color.WHITE
    colorItem.isStroke = stroke
    if (stroke) {
      colorItem.strokeColor = Color.WHITE
    }
    list.add(0, colorItem)
    return list
  }

  inner class ColorItem {
    var isStroke = false
    var color = 0
    var strokeColor = 0
  }

  interface OnItemClickListener {
    fun onItemClick(item: ColorItem?)
  }

  fun setItemClickListener(itemClickListener: OnItemClickListener?) {
    mItemClickListener = itemClickListener
  }

}