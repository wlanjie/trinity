package com.trinity.sample.view

import android.graphics.Rect
import android.view.View
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.StaggeredGridLayoutManager

class SpacesItemDecoration @JvmOverloads constructor(private val mSpace: Int, private val mSpanCount: Int = 1) : RecyclerView.ItemDecoration() {
  private val mRadixX: Int = mSpace / mSpanCount
  private var mItemCountInLastLine: Int = 0
  private var mOldItemCount = -1

  override fun getItemOffsets(outRect: Rect, view: View, parent: RecyclerView, state: RecyclerView.State) {
    val params = view.layoutParams as RecyclerView.LayoutParams
    val sumCount = state.itemCount
    val position = params.viewLayoutPosition
    val spanSize: Int
    val index: Int

    if (params is GridLayoutManager.LayoutParams) {
      spanSize = params.spanSize
      index = params.spanIndex

      if ((position == 0 || mOldItemCount != sumCount) && mSpanCount > 1) {
        var countInLine = 0
        var spanIndex: Int

        for (tempPosition in sumCount - mSpanCount until sumCount) {
          spanIndex = (parent.layoutManager as GridLayoutManager).spanSizeLookup.getSpanIndex(tempPosition, mSpanCount)
          countInLine = if (spanIndex == 0) 1 else countInLine + 1
        }
        mItemCountInLastLine = countInLine
        if (mOldItemCount != sumCount) {
          mOldItemCount = sumCount
          if (position != 0) {
            parent.post { parent.invalidateItemDecorations() }
          }
        }
      }
    } else if (params is StaggeredGridLayoutManager.LayoutParams) {
      spanSize = if (params.isFullSpan) mSpanCount else 1
      index = params.spanIndex
    } else {
      spanSize = 1
      index = 0
    }

    if (spanSize < 1 || index < 0 || spanSize > mSpanCount) {
      return
    }

    outRect.left = mSpace - mRadixX * index
    outRect.right = mRadixX + mRadixX * (index + spanSize - 1)

    if (mSpanCount == 1 && position == sumCount - 1) {
      outRect.bottom = mSpace
    } else if (position >= sumCount - mItemCountInLastLine && position < sumCount) {
      outRect.bottom = mSpace
    }
    outRect.top = mSpace
  }
}