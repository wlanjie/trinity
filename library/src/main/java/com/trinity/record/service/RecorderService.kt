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

package com.trinity.record.service

import com.trinity.record.exception.AudioConfigurationException
import com.trinity.record.exception.StartRecordingException

interface RecorderService {

  /**
   * 获得后台处理数据部分的buffer大小
   */
  val audioBufferSize: Int

  val sampleRate: Int

  /**
   * 清唱的时候获取当前音高 来渲染音量动画
   */
  val recordVolume: Int

  val isPaused: Boolean

  /**
   * 初始化录音器的硬件部分
   */
  @Throws(AudioConfigurationException::class)
  fun init(speed: Float)

  /**
   * 初始化我们后台处理数据部分
   */
  fun initAudioRecorderProcessor(speed: Float): Boolean

  /**
   * 销毁我们的后台处理数据部分
   */
  fun destroyAudioRecorderProcessor()

  /**
   * 开始录音
   */
  @Throws(StartRecordingException::class)
  fun start()

  /**
   * 结束录音
   */
  fun stop()

  fun enableUnAccom()

  fun pause()

  fun continueRecord()

}
