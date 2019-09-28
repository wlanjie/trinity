package com.trinity.sample.adapter

import android.graphics.Bitmap
import android.graphics.Color
import android.util.Log
import android.util.SparseArray
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView

import androidx.recyclerview.widget.RecyclerView

import com.trinity.sample.R
import com.trinity.sample.view.ThumbnailFetcher

class ThumbRecyclerAdapter(private var mCount: Int, duration: Int, private var mFetcher: ThumbnailFetcher?, private val mScreenWidth: Int, thumbnailWidth: Int, thumbnailHeight: Int) : RecyclerView.Adapter<ThumbRecyclerAdapter.ThumbnailViewHolder>() {
  private var mInterval: Long = 0

  private val mCacheBitmaps = SparseArray<Bitmap>()

  init {
    mInterval = (duration / mCount).toLong()
    //        mFetcher.setParameters(thumbnailWidth, thumbnailHeight, ThumbnailFetcher.CropMode.Mediate, VideoDisplayMode.SCALE, 1);
  }

  fun setData(count: Int, duration: Int, fetcher: ThumbnailFetcher, screenWidth: Int, thumbnailWidth: Int, thumbnailHeight: Int) {

    if (mInterval * count != duration.toLong() && mCacheBitmaps.size() != 0) {
      Log.i(TAG, "setData: clear cache")
      mCacheBitmaps.clear()
      cacheBitmaps()
    } else {
      mInterval = (duration / count).toLong()
    }
    this.mFetcher = fetcher
    this.mCount = count
    //        mFetcher.setParameters(thumbnailWidth, thumbnailHeight, ThumbnailFetcher.CropMode.Mediate, VideoDisplayMode.SCALE, 1);

  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ThumbnailViewHolder {
    val holder: ThumbnailViewHolder
    val itemView: View
    when (viewType) {
      VIEW_TYPE_HEADER, VIEW_TYPE_FOOTER -> {
        itemView = View(parent.context)
        itemView.layoutParams = ViewGroup.LayoutParams(mScreenWidth / 2, ViewGroup.LayoutParams.MATCH_PARENT)
        itemView.setBackgroundColor(Color.TRANSPARENT)
        holder = ThumbnailViewHolder(itemView)
        return holder
      }
      else -> {
        itemView = LayoutInflater.from(parent.context).inflate(R.layout.item_timeline_thumbnail, parent, false)
        holder = ThumbnailViewHolder(itemView)
        holder.mIvThumbnail = itemView.findViewById(R.id.iv_thumbnail)
        return holder
      }
    }
  }

  override fun onBindViewHolder(holder: ThumbnailViewHolder, position: Int) {
    if (position != 0 && position != mCount + 1) {
      if (mInterval == 0L) {
        mInterval = (mFetcher?.getTotalDuration() ?: 0 / mCount.toLong())
      }
      requestFetchThumbnail(holder, position)
    }
  }

  private fun requestFetchThumbnail(holder: ThumbnailViewHolder, position: Int) {
    val times = longArrayOf((position - 1) * mInterval + mInterval / 2)

    val bitmap = mCacheBitmaps.get(position)
    if (bitmap != null && !bitmap.isRecycled) {
      holder.mIvThumbnail!!.setImageBitmap(bitmap)
      return
    }
    Log.d(TAG, "requestThumbnailImage() times :" + times[0] + " ,position = " + position)
    mFetcher?.requestThumbnailImage(times, object : ThumbnailFetcher.OnThumbnailCompletionListener {

      private var vecIndex = 1

      override fun onThumbnail(frameBitmap: Bitmap, l: Long) {
        if (frameBitmap != null && !frameBitmap.isRecycled) {
          Log.i(TAG, "onThumbnailReady  put: " + position + " ,l = " + l / 1000)
          holder.mIvThumbnail!!.setImageBitmap(frameBitmap)
          //缓存bitmap
          mCacheBitmaps.put(position, frameBitmap)
        } else {
          if (position == 0) {
            vecIndex = 1
          } else if (position == mCount + 1) {
            vecIndex = -1
          }
          val np = position + vecIndex
          Log.i(TAG, "requestThumbnailImage  failure: thisPosition = " + position + "newPosition = " + np)
          requestFetchThumbnail(holder, np)
        }
      }

      override fun onError(errorCode: Int) {
        Log.w(TAG, "requestThumbnailImage error msg: $errorCode")
      }
    })
  }

  override fun getItemCount(): Int {
    return if (mCount == 0) 0 else mCount + 2//这里加上前后部分的view
  }

  override fun getItemViewType(position: Int): Int {
    return if (position == 0) {
      VIEW_TYPE_HEADER
    } else if (position == mCount + 1) {
      VIEW_TYPE_FOOTER
    } else {
      VIEW_TYPE_THUMBNAIL
    }
  }

  override fun onViewRecycled(holder: ThumbnailViewHolder) {
    super.onViewRecycled(holder)
    if (holder.mIvThumbnail != null) {
      holder.mIvThumbnail!!.setImageBitmap(null)
    }
  }

  /**
   * 缓存缩略图
   */
  fun cacheBitmaps() {
    //与缩略图的长度一致
    for (i in 1 until mCount + 1) {
      requestFetchThumbnail(i)
    }
  }

  /**
   * 提前获取缩略图的
   * @param index count角标
   */
  private fun requestFetchThumbnail(index: Int) {
    val times = longArrayOf((index - 1) * mInterval + mInterval / 2)
    Log.d(TAG, "requestFetchThumbnail请求缓存: ")
    mFetcher!!.requestThumbnailImage(times, object : ThumbnailFetcher.OnThumbnailCompletionListener {
      private var vecIndex = 1//取不到缩略图时再次取值时100毫秒的差值
      override fun onThumbnail(frameBitmap: Bitmap, l: Long) {
        if (frameBitmap != null && !frameBitmap.isRecycled) {
          //缓存bitmap
          mCacheBitmaps.put(index, frameBitmap)
          Log.d(TAG, "缓存ThumbnailReady put，time = " + l / 1000 + ", position = " + index)
        } else {
          if (index == 0) {
            vecIndex = 1
          } else if (index == mCount - 1) {
            vecIndex = -100
          }
          val np = index + vecIndex
          Log.i(TAG, "缓存ThumbnailReady put，failure  time = " + l / 1000)
          requestFetchThumbnail(np)
        }
      }

      override fun onError(errorCode: Int) {
        Log.w(TAG, "requestThumbnailImage error msg: $errorCode")
      }
    })
  }

  inner class ThumbnailViewHolder internal constructor(itemView: View) : RecyclerView.ViewHolder(itemView) {
    internal var mIvThumbnail: ImageView? = null
  }

  companion object {
    private const val TAG = "ThumbRecyclerAdapter"
    private const val VIEW_TYPE_HEADER = 1
    private const val VIEW_TYPE_FOOTER = 2
    private const val VIEW_TYPE_THUMBNAIL = 3
  }
}
