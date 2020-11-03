package com.trinity.sample.adapter

import android.content.Context
import android.net.Uri
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.bumptech.glide.load.resource.bitmap.CircleCrop
import com.bumptech.glide.request.RequestOptions
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.trinity.sample.R
import com.trinity.sample.entity.Filter
import java.io.FileInputStream
import java.nio.charset.Charset

class FilterAdapter(private val context: Context, val callback: (filter: Filter)->Unit) : RecyclerView.Adapter<FilterAdapter.ViewHolder>() {

  private val mFilter: List<Filter>
  private var mSelectPosition = 0

  private inline fun <reified T> genericType() = object: TypeToken<T>() {}.type

  init {
    val stream = FileInputStream(context.externalCacheDir?.absolutePath + "/filter/filter.json")
//    val stream = context.assets.open(context.externalCacheDir?.absolutePath + "filter/filter.json")
    val size = stream.available()
    val buffer = ByteArray(size)
    stream.read(buffer)
    stream.close()
    val gson = Gson()
    mFilter = gson.fromJson<List<Filter>>(String(buffer, Charset.forName("UTF-8")), genericType<List<Filter>>())
  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
    val view = LayoutInflater.from(parent.context).inflate(R.layout.item_filter, parent, false)
    return ViewHolder(view)
  }

  override fun getItemCount(): Int {
    return mFilter.size
  }

  override fun onBindViewHolder(holder: ViewHolder, position: Int) {
    val filter = mFilter[position]
    Glide.with(holder.imageView)
      .load(context.externalCacheDir?.absolutePath + "/filter/${filter.thumbnail}")
      .apply(RequestOptions.bitmapTransform(CircleCrop()))
      .into(holder.imageView)
    if (position == mSelectPosition) {
      holder.selectView.visibility = View.VISIBLE
    } else {
      holder.selectView.visibility = View.INVISIBLE
    }
    holder.textView.text = filter.name
    holder.itemView.setOnClickListener {
      callback(filter)
      mSelectPosition = position
      notifyDataSetChanged()
    }
  }

  class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val imageView: ImageView = itemView.findViewById(R.id.filter_thumbnail)
    val textView: TextView = itemView.findViewById(R.id.filter_name)
    val selectView: View = itemView.findViewById(R.id.select_view)
  }
}