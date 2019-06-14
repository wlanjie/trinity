package com.trinity.sample.adapter

import android.content.Context
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.entity.Music
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import android.provider.MediaStore
import android.view.LayoutInflater
import android.widget.TextView
import com.trinity.sample.R

class MusicAdapter(private val context: Context) : RecyclerView.Adapter<MusicAdapter.ViewHolder>() {

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
//      val title = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.TITLE))
//      var singer = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.ARTIST))
//      if ("<unknown>" == singer) {
//        singer = "未知艺术家"
//      }
//      val album = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.ALBUM))
//      val size = cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Media.SIZE))
        val time = cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Media.DURATION))
        val url = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DATA))
        val name = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DISPLAY_NAME))
        val sbr = name.substring(name.length - 3, name.length)
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
  }

  class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val name = itemView.findViewById<TextView>(R.id.name)
  }
}