package com.trinity.sample

import android.Manifest
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import com.github.florent37.runtimepermission.RuntimePermission
import com.tencent.mars.xlog.Xlog
import com.trinity.sample.entity.MediaItem

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("trinity")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        requestPermission()

        findViewById<Button>(R.id.record)
            .setOnClickListener {
                val intent = Intent(this, RecordActivity::class.java)
                startActivity(intent)
            }

        findViewById<Button>(R.id.editor)
            .setOnClickListener {
                val intent = Intent(this, EditorActivity::class.java)
                val mediaItem = MediaItem("/sdcard/DCIM/Camera/no_audio.mp4", "", 540, 960)
                mediaItem.duration = 14000
                val mediaItems = mutableListOf<MediaItem>()
                mediaItems.add(mediaItem)
                intent.putExtra("medias", mediaItems.toTypedArray())
                startActivity(intent)
            }

        findViewById<Button>(R.id.music)
            .setOnClickListener {
                val intent = Intent(this, MusicActivity::class.java)
                startActivity(intent)
            }
    }

    private fun requestPermission() {
        RuntimePermission.askPermission(this, Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE)
            .onAccepted {

            }
            .onDenied {

            }
    }

}
