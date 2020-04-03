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
import androidx.recyclerview.widget.RecyclerView
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.trinity.sample.R
import com.trinity.sample.entity.Effect
import java.nio.charset.Charset

class IdentifyAdapter(
  context: Context,
  val callback: (effect: Effect?) -> Unit) : RecyclerView.Adapter<IdentifyAdapter.ViewHolder>() {

  private val mEffects: List<Effect>
  private var mCurrentPosition = -1

  init {
    val stream = context.assets.open("identify.json")
    val size = stream.available()
    val buffer = ByteArray(size)
    stream.read(buffer)
    stream.close()
    mEffects = Gson().fromJson(String(buffer, Charset.forName("UTF-8")), object: TypeToken<List<Effect>>(){}.type)
  }

  class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val selectStateImage: ImageView = itemView.findViewById(R.id.select_state)
    val resourceImage: ImageView = itemView.findViewById(R.id.resource_image_view)
  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
    val view = LayoutInflater.from(parent.context).inflate(R.layout.resource_item_view, parent, false)
    return ViewHolder(view)
  }

  override fun getItemCount(): Int {
    return mEffects.size
  }

  override fun onBindViewHolder(holder: ViewHolder, position: Int) {
    if (position != mCurrentPosition) {
      holder.selectStateImage.visibility = View.GONE
    } else {
      holder.selectStateImage.visibility = View.VISIBLE
    }
    holder.itemView.setOnClickListener {
      mCurrentPosition = if (position == mCurrentPosition) {
        callback(null)
        -1
      } else {
        callback(mEffects[position])
        position
      }
      notifyDataSetChanged()
    }
  }
}