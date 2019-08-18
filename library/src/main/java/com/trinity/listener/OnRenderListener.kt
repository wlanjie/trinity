package com.trinity.listener

interface OnRenderListener {
  fun onSurfaceCreated()
  fun onDrawFrame(textureId: Int, width: Int, height: Int, matrix: FloatArray): Int
  fun onSurfaceDestroy()
}