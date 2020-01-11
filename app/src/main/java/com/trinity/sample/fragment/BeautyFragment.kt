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

import android.graphics.Rect
import android.os.Bundle
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R
import com.trinity.sample.adapter.BeautyAdapter
import com.warkiz.widget.IndicatorSeekBar
import com.warkiz.widget.OnSeekChangeListener
import com.warkiz.widget.SeekParams

class BeautyFragment : Fragment() {

  private var mCallback: Callback ?= null

  override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
    return inflater.inflate(R.layout.fragment_beauty, container, false)
  }

  override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
    val recyclerView = view.findViewById<RecyclerView>(R.id.recycler_view)
    recyclerView.layoutManager = LinearLayoutManager(activity, LinearLayoutManager.HORIZONTAL, false)
    recyclerView.adapter = BeautyAdapter {
    }
    recyclerView.addItemDecoration(object: RecyclerView.ItemDecoration() {
      override fun getItemOffsets(outRect: Rect, view: View, parent: RecyclerView, state: RecyclerView.State) {
        outRect.left = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 5f, resources.displayMetrics).toInt()
        outRect.right = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 5f, resources.displayMetrics).toInt()
      }
    })
    val seekBar = view.findViewById<IndicatorSeekBar>(R.id.seek_bar)
    seekBar.onSeekChangeListener = object: OnSeekChangeListener {
      override fun onSeeking(seekParams: SeekParams) {
        mCallback?.onBeautyParam(seekParams.progress, 0)
      }

      override fun onStartTrackingTouch(seekBar: IndicatorSeekBar?) {
      }

      override fun onStopTrackingTouch(seekBar: IndicatorSeekBar?) {
      }

    }
  }

  fun setCallback(callback: Callback) {
    mCallback = callback
  }

  interface Callback {
    fun onBeautyParam(value: Int, position: Int)
  }
}