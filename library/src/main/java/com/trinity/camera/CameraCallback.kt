package com.trinity.camera

interface CameraCallback {

  fun onPreviewSizeSelected(sizes: List<Size>): Size
}