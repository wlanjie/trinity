package com.trinity.sample.view

import android.content.Context
import android.util.AttributeSet
import android.widget.HorizontalScrollView

/**
 * Created by wlanjie on 2019-07-25
 */
class EditorTabLayout : HorizontalScrollView {

    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
        init()
    }

    private fun init() {

    }

    fun addTab() {

    }

    class Tab {

    }
}