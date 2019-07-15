package com.trinity.camera

import java.util.*

interface CameraCallback {

  fun onPreviewSizeSelected(sizes: SortedSet<Size>): Size?
}