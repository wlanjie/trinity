package com.trinity.sample

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.SurfaceView
import com.trinity.Record

class MainActivity : AppCompatActivity() {

    private lateinit var mRecord: Record

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        System.loadLibrary("trinity")

        val surfaceView = findViewById<SurfaceView>(R.id.surface_view)
        mRecord = Record(this)
        mRecord.setView(surfaceView)
    }

    override fun onStop() {
        super.onStop()
//        mRecord.stop()
    }
}
