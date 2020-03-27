/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package com.trinity.camera

import android.os.Parcel
import android.os.Parcelable

class AspectRatio private constructor(val x: Int, val y: Int) : Comparable<AspectRatio>, Parcelable {

  fun matches(size: Size): Boolean {
    val gcd = gcd(size.width, size.height)
    val x = size.width / gcd
    val y = size.height / gcd
    return this.x == x && this.y == y
  }

  override fun equals(other: Any?): Boolean {
    if (other == null) {
      return false
    }
    if (this === other) {
      return true
    }
    if (other is AspectRatio) {
      return x == other.x && y == other.y
    }
    return false
  }

  override fun toString(): String {
    return "$x:$y"
  }

  fun toFloat(): Float {
    return x.toFloat() / y
  }

  fun flip(): AspectRatio {
    return AspectRatio(y, x)
  }

  override fun hashCode(): Int {
    // assuming most sizes are <2^16, doing a rotate will give us perfect hashing
    return y xor (x shl Integer.SIZE / 2 or x.ushr(Integer.SIZE / 2))
  }

  override fun compareTo(other: AspectRatio): Int {
    if (equals(other)) {
      return 0
    } else if (toFloat() - other.toFloat() > 0) {
      return 1
    }
    return -1
  }

  /**
   * @return The inverse of this [AspectRatio].
   */
  @Suppress("unused")
  fun inverse(): AspectRatio {
    return of(y, x)
  }

  override fun describeContents(): Int {
    return 0
  }

  override fun writeToParcel(dest: Parcel, flags: Int) {
    dest.writeInt(x)
    dest.writeInt(y)
  }

  companion object {

    private val sCache = mutableMapOf<String, AspectRatio>()

    fun of(size: Size): AspectRatio {
      return of(size.width, size.height)
    }

    /**
     * Returns an instance of [AspectRatio] specified by `x` and `y` values.
     * The values `x` and `` will be reduced by their greatest common divider.
     *
     * @param a The width
     * @param b The height
     * @return An instance of [AspectRatio]
     */
    fun of(a: Int, b: Int): AspectRatio {
      var x = a
      var y = b
      val gcd = gcd(x, y)
      x /= gcd
      y /= gcd
      val key = "$x:$y"
      var cached = sCache[key]
      if (cached == null) {
        cached = AspectRatio(x, y)
        sCache[key] = cached
      }
      return cached
    }

    /**
     * Parse an [AspectRatio] from a [String] formatted like "4:3".
     *
     * @param s The string representation of the aspect ratio
     * @return The aspect ratio
     * @throws IllegalArgumentException when the format is incorrect.
     */
    @Suppress("unused")
    fun parse(s: String): AspectRatio {
      val position = s.indexOf(':')
      if (position == -1) {
        throw IllegalArgumentException("Malformed aspect ratio: $s")
      }
      try {
        val x = Integer.parseInt(s.substring(0, position))
        val y = Integer.parseInt(s.substring(position + 1))
        return of(x, y)
      } catch (e: NumberFormatException) {
        throw IllegalArgumentException("Malformed aspect ratio: $s", e)
      }

    }

    private fun gcd(x: Int, y: Int): Int {
      var a = x
      var b = y
      while (b != 0) {
        val c = b
        b = a % b
        a = c
      }
      return a
    }

    @JvmField
    @Suppress("unused")
    val CREATOR = object : Parcelable.Creator<AspectRatio> {

      override fun createFromParcel(source: Parcel): AspectRatio {
        val x = source.readInt()
        val y = source.readInt()
        return of(x, y)
      }

      override fun newArray(size: Int): Array<AspectRatio?> {
        return arrayOfNulls(size)
      }
    }
  }

}
