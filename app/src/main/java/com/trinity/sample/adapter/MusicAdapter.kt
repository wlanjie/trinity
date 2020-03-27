package com.trinity.sample.adapter

import android.content.Context
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.entity.Music
import android.provider.MediaStore
import android.view.LayoutInflater
import android.widget.TextView
import com.trinity.sample.R
import kotlinx.coroutines.*

class MusicAdapter(private val context: Context, private val callback: (music: String) -> Unit) : RecyclerView.Adapter<MusicAdapter.ViewHolder>() {

  private var mMusics = mutableListOf<Music>()

  private suspend fun getMusic() : List<Music> = GlobalScope.async {
    val musics = mutableListOf<Music>()
    val resolver = context.contentResolver
    val cursor = resolver.query(
      MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, null, null, null,
      MediaStore.Audio.Media.DEFAULT_SORT_ORDER
    )
    if (cursor?.moveToFirst() == true) {
      do {
        val time = cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Media.DURATION))
        val url = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DATA))
        val name = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DISPLAY_NAME))
        val sbr = url.substring(url.length - 3, url.length)
        if ("mp3" == sbr) {
          val m = Music(url, name, time)
          musics.add(m)
        }
      } while (cursor.moveToNext())
    }
    cursor?.close()
    return@async musics
  }.await()

  init {
    GlobalScope.launch {
      val muiscs = getMusic()
      mMusics.clear()
      mMusics.addAll(muiscs)
      withContext(Dispatchers.Main) {
        notifyDataSetChanged()
      }
    }
  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
    val inflater = LayoutInflater.from(parent.context)
    return ViewHolder(inflater.inflate(R.layout.item_music, parent, false))
  }

  override fun getItemCount(): Int {
    return mMusics.size
  }

  override fun onBindViewHolder(holder: ViewHolder, position: Int) {
    val music = mMusics[position]
    holder.name.text = music.name
    holder.itemView.setOnClickListener {
      callback(music.path)
    }
  }

  class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val name = itemView.findViewById<TextView>(R.id.name)
  }
}