package com.trinity.camera

import android.graphics.PointF

interface CameraCallback {
  fun dispatchOnFocusStart(where: PointF)
  fun dispatchOnFocusEnd(success: Boolean, where: PointF)
}