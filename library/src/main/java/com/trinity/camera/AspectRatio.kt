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

  override fun equals(o: Any?): Boolean {
    if (o == null) {
      return false
    }
    if (this === o) {
      return true
    }
    if (o is AspectRatio) {
      val ratio = o as AspectRatio?
      return x == ratio!!.x && y == ratio.y
    }
    return false
  }

  override fun toString(): String {
    return "$x:$y"
  }

  fun toFloat(): Float {
    return x.toFloat() / y
  }

  override fun hashCode(): Int {
    // assuming most sizes are <2^16, doing a rotate will give us perfect hashing
    return y xor (x shl Integer.SIZE / 2 or x.ushr(Integer.SIZE / 2))
  }

  override fun compareTo(another: AspectRatio): Int {
    if (equals(another)) {
      return 0
    } else if (toFloat() - another.toFloat() > 0) {
      return 1
    }
    return -1
  }

  /**
   * @return The inverse of this [AspectRatio].
   */
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

    private val sCache = mutableMapOf<Int, HashMap<Int, AspectRatio>>()

    /**
     * Returns an instance of [AspectRatio] specified by `x` and `y` values.
     * The values `x` and `` will be reduced by their greatest common divider.
     *
     * @param x The width
     * @param y The height
     * @return An instance of [AspectRatio]
     */
    fun of(a: Int, b: Int): AspectRatio {
      var x = a
      var y = b
      val gcd = gcd(x, y)
      x /= gcd
      y /= gcd
      var arrayX = sCache[x]
      return if (arrayX == null) {
        val ratio = AspectRatio(x, y)
        arrayX = HashMap<Int, AspectRatio>()
        arrayX[y] = ratio
        sCache[x] = arrayX
        ratio
      } else {
        var ratio = arrayX[y]
        if (ratio == null) {
          ratio = AspectRatio(x, y)
          arrayX[y] = ratio
        }
        ratio
      }
    }

    /**
     * Parse an [AspectRatio] from a [String] formatted like "4:3".
     *
     * @param s The string representation of the aspect ratio
     * @return The aspect ratio
     * @throws IllegalArgumentException when the format is incorrect.
     */
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
