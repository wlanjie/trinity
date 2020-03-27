/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
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
 *
 */

package com.trinity.camera

import android.content.Context
import android.util.AttributeSet
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceHolder.Callback
import android.view.SurfaceView

class TrinityPreviewView : SurfaceView, Callback {

  constructor(context: Context) : super(context)

  constructor(context: Context, set: AttributeSet) : super(context, set)

  constructor(context: Context, set: AttributeSet, defStyleAttr: Int) : super(context, set, defStyleAttr)

  private var mCallback: PreviewViewCallback? = null

  init {
    val surfaceHolder = holder
    surfaceHolder.addCallback(this)
//    surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS)
  }

  override fun surfaceCreated(holder: SurfaceHolder) {
    val surface = holder.surface
    val width = width
    val height = height
    mCallback?.createSurface(surface, width, height)
  }

  override fun surfaceChanged(
    holder: SurfaceHolder, format: Int, width: Int,
    height: Int
  ) {
    mCallback?.resetRenderSize(width, height)
  }

  override fun surfaceDestroyed(holder: SurfaceHolder) {
    mCallback?.destroySurface()
  }

  fun setCallback(callback: PreviewViewCallback) {
    this.mCallback = callback
  }

  interface PreviewViewCallback {
    fun createSurface(surface: Surface, width: Int, height: Int)
    fun resetRenderSize(width: Int, height: Int)
    fun destroySurface()
  }
}
