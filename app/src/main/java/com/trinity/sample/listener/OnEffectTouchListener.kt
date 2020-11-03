package com.trinity.sample.listener

import com.trinity.sample.entity.Effect

interface OnEffectTouchListener {

  fun onEffectTouchEvent(event: Int, effect: Effect)
}