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

import android.content.Context
import com.alibaba.android.mnnkit.actor.FaceDetector
import com.alibaba.android.mnnkit.entity.FaceDetectConfig
import com.alibaba.android.mnnkit.entity.MNNCVImageFormat
import com.alibaba.android.mnnkit.entity.MNNFlipType
import com.alibaba.android.mnnkit.intf.InstanceCreatedListener
import com.alibaba.android.mnnkit.monitor.MNNMonitor
import com.tencent.mars.xlog.Log

class MnnFaceDetection : FaceDetection {

  private var mFaceDetector: FaceDetector ?= null
  private val mFaceDetectionReports = Array(20) { FaceDetectionReport() }

  override fun createFaceDetection(context: Context, type: Int): Int {
    MNNMonitor.setMonitorEnable(false)
    val config = FaceDetector.FaceDetectorCreateConfig()
    config.mode = if (type == 0) FaceDetector.FaceDetectMode.MOBILE_DETECT_MODE_VIDEO else FaceDetector.FaceDetectMode.MOBILE_DETECT_MODE_IMAGE
    FaceDetector.createInstanceAsync(context, config, object: InstanceCreatedListener<FaceDetector> {
      override fun onFailed(p0: Int, error: Error?) {
        Log.e("trinity", "error: ${error?.message}")
      }

      override fun onSucceeded(faceDetector: FaceDetector?) {
        mFaceDetector = faceDetector
      }

    })
    return 0
  }

  override fun faceDetection(data: ByteArray, width: Int, height: Int,
                             faceDetectionImageType: FaceDetectionImageType,
                             inAngle: Int, outAngle: Int, flipType: FlipType): Array<com.trinity.face.FaceDetectionReport> {
    val outputFlip = when (flipType) {
      FlipType.FLIP_NONE -> {
        MNNFlipType.FLIP_NONE
      }
      FlipType.FLIP_X -> {
        MNNFlipType.FLIP_X
      }
      FlipType.FLIP_Y -> {
        MNNFlipType.FLIP_Y
      }
      FlipType.FLIP_XY -> {
        MNNFlipType.FLIP_XY
      }
    }
    val detectConfig = FaceDetectConfig.ACTIONTYPE_EYE_BLINK or FaceDetectConfig.ACTIONTYPE_MOUTH_AH or FaceDetectConfig.ACTIONTYPE_HEAD_YAW or FaceDetectConfig.ACTIONTYPE_HEAD_PITCH or FaceDetectConfig.ACTIONTYPE_BROW_JUMP
    val result = mFaceDetector?.inference(data, width, height, MNNCVImageFormat.YUV_NV21, detectConfig, inAngle, outAngle, outputFlip)
    val size = result?.size ?: 0
    val faceDetectionReports = Array(result?.size ?: 0) {FaceDetectionReport()}
    if (size >= 20) {

    } else {
      result?.forEachIndexed { index, faceDetectionReport ->
        val faceDetection = faceDetectionReports[index]
//        val faceDetection = FaceDetectionReport()
        faceDetectionReport.rect?.let {
          faceDetection.left = it.left
          faceDetection.right = it.right
          faceDetection.top = it.top
          faceDetection.bottom = it.bottom
        }
        faceDetection.faceId = faceDetectionReport.faceID
        faceDetection.keyPoints = faceDetectionReport.keyPoints
        faceDetection.visibilities = faceDetectionReport.visibilities
        faceDetection.score = faceDetectionReport.score
        faceDetection.yaw = faceDetectionReport.yaw
        faceDetection.pitch = faceDetectionReport.pitch
        faceDetection.roll = faceDetectionReport.roll
        faceDetection.faceAction = faceDetectionReport.faceAction
        faceDetection.faceActionMap = faceDetectionReport.faceActionMap
        faceDetectionReports[index] = faceDetection
      }
    }
    return faceDetectionReports
  }

  override fun releaseDetection() {
    mFaceDetector?.release()
  }
}