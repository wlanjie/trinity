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

import java.util.*
import kotlin.collections.ArrayList

/**
 * Created by wlanjie on 2019-07-16
 */
object SizeSelectors {

  interface Filter {
    fun accepts(size: Size): Boolean
  }

  fun withFilter(filter: Filter): SizeSelector {
    return FilterSelector(filter)
  }

  fun maxWidth(width: Int): SizeSelector {
    return withFilter(object: Filter {
      override fun accepts(size: Size): Boolean {
        return size.width <= width
      }
    })
  }

  fun minWidth(width: Int): SizeSelector {
    return withFilter(object: Filter {
      override fun accepts(size: Size): Boolean {
        return size.width >= width
      }
    })
  }

  fun maxHeight(height: Int): SizeSelector {
    return withFilter(object: Filter {
      override fun accepts(size: Size): Boolean {
        return size.height <= height
      }
    })
  }

  fun minHeight(height: Int): SizeSelector {
    return withFilter(object: Filter {
      override fun accepts(size: Size): Boolean {
        return size.height >= height
      }
    })
  }

  fun aspectRatio(ratio: AspectRatio?, delta: Float): SizeSelector {
    val desired = ratio?.toFloat() ?: 0f
    return withFilter(object: Filter {
      override fun accepts(size: Size): Boolean {
        val candidate = AspectRatio.of(size.width, size.height).toFloat()
        return candidate >= desired - delta && candidate <= desired + delta
      }
    })
  }

  fun biggest(): SizeSelector {
    return object : SizeSelector {
      override fun select(source: List<Size>): List<Size> {
        Collections.sort(source)
        Collections.reverse(source)
        return source
      }
    }
  }

  fun smallest(): SizeSelector {
    return object: SizeSelector {
      override fun select(source: List<Size>): List<Size> {
        Collections.sort(source)
        return source
      }
    }
  }

  fun maxArea(area: Int): SizeSelector {
    return withFilter(object: Filter {
      override fun accepts(size: Size): Boolean {
        return size.height * size.width >= area
      }
    })
  }

  fun and(vararg selector: SizeSelector): SizeSelector {
    val values = mutableListOf<SizeSelector>()
    selector.forEach {
      values.add(it)
    }
    return AndSelector(values)
  }

  fun or(vararg selector: SizeSelector): SizeSelector {
    val values = mutableListOf<SizeSelector>()
    selector.forEach {
      values.add(it)
    }
    return OrSelector(values)
  }

  private class FilterSelector(
    private val constraint: Filter
  ) : SizeSelector {

    override fun select(source: List<Size>): List<Size> {
      val sizes = ArrayList<Size>()
      source.forEach {
        if (constraint.accepts(it)) {
          sizes.add(it)
        }
      }
      return sizes
    }
  }

  private class OrSelector(
    private val values: List<SizeSelector>
  ) : SizeSelector {
    override fun select(source: List<Size>): List<Size> {
      var temp: List<Size> ?= null
      for (selector in values) {
        temp = selector.select(source)
        if (temp.isNotEmpty()) {
          break
        }
      }
      return temp ?: ArrayList()
    }
  }

  private class AndSelector(
    private val values: List<SizeSelector>
  ) : SizeSelector {
    override fun select(source: List<Size>): List<Size> {
      var temp = source
      for (selector in values) {
        temp = selector.select(temp)
      }
      return temp
    }
  }
}