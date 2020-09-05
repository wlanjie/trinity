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
 * Created by wlanjie on 2019-07-15
 */
class Angles {

  private var mSensorFacing: Facing? = null
  internal var mSensorOffset = 0
  internal var mDisplayOffset = 0
  internal var mDeviceOrientation = 0

  /**
   * We want to keep everything in the [Axis.ABSOLUTE] reference,
   * so a front facing sensor offset must be inverted.
   *
   * @param sensorFacing sensor facing value
   * @param sensorOffset sensor offset
   */
  fun setSensorOffset(sensorFacing: Facing, sensorOffset: Int) {
    sanitizeInput(sensorOffset)
    mSensorFacing = sensorFacing
    mSensorOffset = sensorOffset
    if (mSensorFacing === Facing.FRONT) {
      mSensorOffset = sanitizeOutput(360 - mSensorOffset)
    }
  }

  /**
   * Sets the display offset.
   * @param displayOffset the display offset
   */
  fun setDisplayOffset(displayOffset: Int) {
    sanitizeInput(displayOffset)
    mDisplayOffset = displayOffset
  }

  /**
   * Sets the device orientation.
   * @param deviceOrientation the device orientation
   */
  fun setDeviceOrientation(deviceOrientation: Int) {
    sanitizeInput(deviceOrientation)
    mDeviceOrientation = deviceOrientation
  }

  /**
   * Returns the offset between two reference systems, computed along the given axis.
   * @param from the source reference system
   * @param to the destination reference system
   * @param axis the axis
   * @return the offset
   */
  fun offset(from: Reference, to: Reference, axis: Axis): Int {
    var offset = absoluteOffset(from, to)
    if (axis === Axis.RELATIVE_TO_SENSOR) {
      if (mSensorFacing === Facing.FRONT) {
        offset = sanitizeOutput(360 - offset)
      }
    }
    return offset
  }

  private fun absoluteOffset(from: Reference, to: Reference): Int {
    return if (from === to) {
      0
    } else if (to === Reference.BASE) {
      sanitizeOutput(360 - absoluteOffset(to, from))
    } else if (from === Reference.BASE) {
      when (to) {
        Reference.VIEW -> sanitizeOutput(360 - mDisplayOffset)
        Reference.OUTPUT -> sanitizeOutput(mDeviceOrientation)
        Reference.SENSOR -> sanitizeOutput(360 - mSensorOffset)
        else -> throw RuntimeException("Unknown reference: $to")
      }
    } else {
      sanitizeOutput(
        absoluteOffset(Reference.BASE, to) - absoluteOffset(Reference.BASE, from)
      )
    }
  }

  /**
   * Whether the two references systems are flipped.
   * @param from source
   * @param to destination
   * @return true if flipped
   */
  fun flip(from: Reference, to: Reference): Boolean {
    return offset(from, to, Axis.ABSOLUTE) % 180 != 0
  }

  private fun sanitizeInput(value: Int) {
    if (value != 0
      && value != 90
      && value != 180
      && value != 270
    ) {
      throw IllegalStateException("This value is not sanitized: $value")
    }
  }

  private fun sanitizeOutput(value: Int): Int {
    return (value + 360) % 360
  }
}