/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
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
 */

package com.trinity.face

class FaceDetectionReport {
  var left = 0
  var right = 0
  var top = 0
  var bottom = 0
  var faceId = 0
  var keyPoints: FloatArray = floatArrayOf()
  var visibilities: FloatArray = floatArrayOf()
  var score = 0f
  var yaw = 0f
  var pitch = 0f
  var roll = 0f
  var faceAction = 0L
  var faceActionMap = mapOf<String, Boolean>()
}