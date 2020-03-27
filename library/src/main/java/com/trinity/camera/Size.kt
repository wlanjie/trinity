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

/**
 *  Created by wlanjie on 2019-07-15
 */
class Size(val width: Int, val height: Int) : Comparable<Size> {

  fun flip(): Size {
    return Size(height, width)
  }

  override fun equals(other: Any?): Boolean {
    if (other == null) {
      return false
    }
    if (this === other) {
      return true
    }
    if (other is Size) {
      return width == other.width && height == other.height
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

  override fun compareTo(other: Size): Int {
    return width * height - other.width * other.height
  }

}
