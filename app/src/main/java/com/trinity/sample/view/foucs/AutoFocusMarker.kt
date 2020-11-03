package com.trinity.sample.view.foucs

import android.graphics.PointF

interface AutoFocusMarker : Marker {

  fun onAutoFocusStart(trigger: AutoFocusTrigger, point: PointF)

  fun onAutoFocusEnd(trigger: AutoFocusTrigger, successful: Boolean, point: PointF)
}
