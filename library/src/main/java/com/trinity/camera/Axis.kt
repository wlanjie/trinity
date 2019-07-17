package com.trinity.camera

enum class Axis {

  /**
   * This rotation axis is the one going out of the device screen
   * towards the user's face.
   */
  ABSOLUTE,

  /**
   * This rotation axis takes into account the current
   */
  RELATIVE_TO_SENSOR
}
