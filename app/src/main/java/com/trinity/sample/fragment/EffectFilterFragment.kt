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

package com.trinity.sample.fragment

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.trinity.sample.R
import com.trinity.sample.adapter.EffectAdapter
import com.trinity.sample.entity.Effect
import java.nio.charset.Charset

class EffectFilterFragment : Fragment() {

  private var mAdapter: EffectAdapter ?= null
  private var mOnItemTouchListener: EffectAdapter.OnItemTouchListener ?= null

  companion object {

    private const val CONTENT = "content"

    fun newInstance(content: String): EffectFilterFragment {
      val fragment = EffectFilterFragment()
      val bundle = Bundle()
      bundle.putString(CONTENT, content)
      fragment.arguments = bundle
      return fragment
    }
  }

  override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
    return inflater.inflate(R.layout.recycler_view, container, false)
  }

  override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
    super.onViewCreated(view, savedInstanceState)
    val recyclerView = view as RecyclerView
    val content = arguments?.getString(CONTENT) ?: return
    val stream = view.context.assets.open(content)
    val size = stream.available()
    val buffer = ByteArray(size)
    stream.read(buffer)
    stream.close()
    val effects = Gson().fromJson<List<Effect>>(String(buffer, Charset.forName("UTF-8")), object: TypeToken<List<com.trinity.sample.entity.Effect>>(){}.type)
    mAdapter = EffectAdapter(view.context, effects)
    mAdapter?.setOnTouchListener(mOnItemTouchListener)
    recyclerView.adapter = mAdapter
    recyclerView.layoutManager = LinearLayoutManager(view.context, RecyclerView.HORIZONTAL, false)
  }

  fun setOnItemTouchListener(l: EffectAdapter.OnItemTouchListener) {
    mOnItemTouchListener = l
    mAdapter?.setOnTouchListener(l)
  }
}