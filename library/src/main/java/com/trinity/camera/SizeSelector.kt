package com.trinity.camera

/**
 * Created by wlanjie on 2019-07-16
 */
interface SizeSelector {

  fun select(source: List<Size>): List<Size>
}