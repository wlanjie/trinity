package com.trinity.camera

/**
 *  Created by wlanjie on 2019-07-15
 */
class Size(val width: Int, val height: Int) : Comparable<Size> {

  override fun equals(o: Any?): Boolean {
    if (o == null) {
      return false
    }
    if (this === o) {
      return true
    }
    if (o is Size) {
      val size = o as Size?
      return width == size!!.width && height == size.height
    }
    return false
  }

  override fun toString(): String {
    return width.toString() + "x" + height
  }

  override fun hashCode(): Int {
    // assuming most sizes are <2^16, doing a rotate will give us perfect hashing
    return height xor (width shl Integer.SIZE / 2 or width.ushr(Integer.SIZE / 2))
  }

  override fun compareTo(another: Size): Int {
    return width * height - another.width * another.height
  }

}
