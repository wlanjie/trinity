package com.trinity.sample.editor

import android.content.Context
import android.graphics.Rect
import android.util.AttributeSet
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.View
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R
import com.trinity.sample.adapter.FilterAdapter
import com.trinity.sample.entity.Filter

/**
 * Created by wlanjie on 2019-07-24
 */
class LutFilterChooser : Chooser {

  private var mCallback: ((Filter) -> Unit?)? = null

  constructor(context: Context, callback: (filter: Filter)->Unit) : super(context) {
    mCallback = callback
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs)

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr)

  override fun init() {
    LayoutInflater.from(context).inflate(R.layout.lut_filter_view, this)
    val recyclerView = findViewById<RecyclerView>(R.id.effect_list_filter);
    val layoutManager = LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL, false)
    recyclerView.layoutManager = layoutManager
    recyclerView.adapter = FilterAdapter(context) {
      mCallback?.let { it1 -> it1(it) }
    }
    recyclerView.addItemDecoration(object: RecyclerView.ItemDecoration() {
      override fun getItemOffsets(outRect: Rect, view: View, parent: RecyclerView, state: RecyclerView.State) {
        outRect.left = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 5f, resources.displayMetrics).toInt()
        outRect.right = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 5f, resources.displayMetrics).toInt()
      }
    })
  }

  override fun isPlayerNeedZoom(): Boolean {
    return false
  }
}