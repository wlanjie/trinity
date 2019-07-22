package com.trinity.sample

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.adapter.MusicAdapter

class MusicActivity : AppCompatActivity() {

  private lateinit var mMusicRecycler: RecyclerView

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    setContentView(R.layout.activity_music)
    mMusicRecycler = findViewById(R.id.music_recycler)
    mMusicRecycler.adapter = MusicAdapter(this) {
      val intent = Intent()
      intent.putExtra("music", it)
      setResult(Activity.RESULT_OK, intent)
      finish()
    }
    mMusicRecycler.layoutManager = LinearLayoutManager(this, RecyclerView.VERTICAL, false)
  }
}