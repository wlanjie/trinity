package com.trinity.sample.editor

import android.content.Context
import android.util.AttributeSet

/**
 * Created by wlanjie on 2019-07-24
 */
class MusicChooser : Chooser {

  constructor(context: Context) : super(context)

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs)

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr)

  override fun init() {
  }

  override fun isPlayerNeedZoom(): Boolean {
    return false
  }
}