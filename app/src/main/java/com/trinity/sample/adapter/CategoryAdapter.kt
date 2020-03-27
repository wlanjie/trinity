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
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R
import com.trinity.sample.entity.EffectInfo
import com.trinity.sample.entity.ResourceForm
import de.hdodenhof.circleimageview.CircleImageView

class CategoryAdapter(
    private val context: Context,
    private val onMoreCallback: () -> Unit
) : RecyclerView.Adapter<CategoryAdapter.CategoryViewHolder>(), View.OnClickListener {

  private var mSelectedPosition = 0
  private val mResources = mutableListOf<ResourceForm>()
  private var mIsShowFontCategory = false

  companion object {
    private const val VIEW_TYPE_SELECTED = 1
    private const val VIEW_TYPE_UNSELECTED = 2
  }

  fun setData(data: MutableList<ResourceForm>) {
    mResources.addAll(data)
    if (mIsShowFontCategory) {
      val from = ResourceForm()
      mResources.add(0, from)
    }
    notifyDataSetChanged()
  }

  fun addShowFontCategory() {
    mIsShowFontCategory = true
  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): CategoryViewHolder {
    val view = LayoutInflater.from(parent.context)
        .inflate(R.layout.item_category_view, parent, false)
    return CategoryViewHolder(view)
  }

  override fun getItemCount(): Int {
    return mResources.size
  }

  override fun onBindViewHolder(holder: CategoryViewHolder, position: Int) {
    val form = mResources[position]
    val viewType = getItemViewType(position)
    if (position == 0 && mIsShowFontCategory) {
      holder.name.visibility = View.VISIBLE
      holder.image.visibility = View.GONE
      holder.name.text = "纯文字"
    } else if (form.isMore) {
      holder.name.visibility = View.GONE
      holder.image.visibility = View.VISIBLE
      holder.image.setImageResource(R.mipmap.ic_more)
    } else {
      holder.image.visibility = View.GONE
      holder.name.visibility = View.VISIBLE
      holder.name.text = form.name
    }
    holder.itemView.tag = holder
    holder.itemView.setOnClickListener(this)
    when (viewType) {
      VIEW_TYPE_SELECTED -> {
        holder.itemView.isSelected = true
      }
      VIEW_TYPE_UNSELECTED -> holder.itemView.isSelected = false
    }
  }

  fun selectPosition(categoryIndex: Int) {
    val lasPos = mSelectedPosition
    mSelectedPosition = categoryIndex
    notifyItemChanged(mSelectedPosition)
    notifyItemChanged(lasPos)
  }

  override fun onClick(v: View) {
    val viewHolder = v.tag as CategoryViewHolder
    val position = viewHolder.adapterPosition
    val form: ResourceForm = mResources[position]
    val type = getItemViewType(position)
    if (form.isMore) {
      onMoreCallback()
    } else {
      val effectInfo = EffectInfo()
//        effectInfo.isCategory = true
      val lastPos = mSelectedPosition
      mSelectedPosition = viewHolder.adapterPosition
//        mItemClick.onItemClick(effectInfo, viewHolder.adapterPosition)
      notifyItemChanged(lastPos)
      notifyItemChanged(mSelectedPosition)
    }
  }

  override fun getItemViewType(position: Int): Int {
    return if (position == mSelectedPosition) VIEW_TYPE_SELECTED else VIEW_TYPE_UNSELECTED
  }

  class CategoryViewHolder(view: View) : RecyclerView.ViewHolder(view) {
    val image: ImageView = view.findViewById(R.id.category_image)
    val name: TextView = view.findViewById(R.id.category_name)
  }
}