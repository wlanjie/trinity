package com.trinity.sample.adapter

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.bumptech.glide.load.engine.DiskCacheStrategy
import com.bumptech.glide.request.RequestOptions
import com.trinity.sample.R
import com.trinity.sample.entity.MediaItem

class MediaAdapter : RecyclerView.Adapter<MediaAdapter.ViewHolder>() {

  private val mMedias = mutableListOf<MediaItem>()
  private val mSelectMedias = mutableListOf<MediaItem>()

  fun addMedias(medias: MutableList<MediaItem>) {
    mMedias.clear()
    mMedias.addAll(medias)
    notifyDataSetChanged()
  }

  fun getSelectMedia(): MutableList<MediaItem> {
    return mSelectMedias
  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
    val inflater = LayoutInflater.from(parent.context)
    val view = inflater.inflate(R.layout.item_media, parent, false)
    return ViewHolder(view)
  }

  override fun getItemCount(): Int {
    return mMedias.size
  }

  override fun onBindViewHolder(holder: ViewHolder, position: Int) {
    val item = mMedias[position]
    val options = RequestOptions()
        .diskCacheStrategy(DiskCacheStrategy.ALL)
        .centerCrop()

    Glide.with(holder.itemView.context)
        .asBitmap()
        .load(item.path)
        .apply(options)
        .into(holder.imageView)

    var index = -1
    mSelectMedias.forEachIndexed { i, mediaItem ->
      if (mediaItem.path == item.path) {
        index = i
      }
    }
    if (index >= 0) {
      holder.textView.text = (index + 1).toString()
      holder.textView.setBackgroundResource(R.drawable.media_select)
    } else {
      holder.textView.text = ""
      holder.textView.setBackgroundResource(R.drawable.media_select_normal)
    }

    holder.textView.setOnClickListener {
      if (mSelectMedias.contains(item)) {
        mSelectMedias.remove(item)
      } else {
        mSelectMedias.add(item)
      }
      notifyDataSetChanged()
    }
  }

  inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val imageView: ImageView = itemView.findViewById(R.id.media_image)
    val textView: TextView = itemView.findViewById(R.id.select_count)
  }
}