package com.trinity.sample.listener

import com.trinity.sample.entity.EffectInfo

interface OnEffectTouchListener {

  fun onEffectTouchEvent(event: Int, info: EffectInfo)
}