package com.trinity.camera

import android.content.Context
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceHolder.Callback
import android.view.SurfaceView

class TrinityPreviewView(context: Context) : SurfaceView(context), Callback {

    private var mCallback: PreviewViewCallback? = null

    init {
        val surfaceHolder = holder
        surfaceHolder.addCallback(this)
        surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        val surface = holder.surface
        val width = width
        val height = height
        mCallback?.createSurface(surface, width, height)
    }

    override fun surfaceChanged(
        holder: SurfaceHolder, format: Int, width: Int,
        height: Int
    ) {
        mCallback?.resetRenderSize(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        mCallback?.destroySurface()
    }

    fun setCallback(callback: PreviewViewCallback) {
        this.mCallback = callback
    }

    interface PreviewViewCallback {
        fun createSurface(surface: Surface, width: Int, height: Int)
        fun resetRenderSize(width: Int, height: Int)
        fun destroySurface()
    }
}
