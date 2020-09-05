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

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R

class BeautyAdapter(val callback: (position: Int) -> Unit) : RecyclerView.Adapter<BeautyAdapter.ViewHolder>() {

  private val mImages = arrayOf(R.mipmap.ic_smoother)
  private val mTexts = arrayOf(R.string.smoother)

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
    val view = LayoutInflater.from(parent.context).inflate(R.layout.item_beauty, parent, false)
    return ViewHolder(view)
  }

  override fun getItemCount(): Int {
    return mImages.size
  }

  override fun onBindViewHolder(holder: ViewHolder, position: Int) {
    holder.imageView.setImageResource(mImages[position])
    holder.textView.text = holder.itemView.resources.getText(mTexts[position])
  }

  class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val imageView: ImageView = itemView.findViewById(R.id.item_beauty_image)
    val textView: TextView = itemView.findViewById(R.id.item_beauty_text)
  }
}