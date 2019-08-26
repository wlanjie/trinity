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

package com.trinity.decoder

class MusicDecoder : Mp3Decoder {

  private var mId: Long = 0

  init {
    mId = create()
  }


  override fun init(packetBufferTimePercent: Float, vocalSampleRate: Int) {
    if (mId <= 0) {
      return
    }
    initDecoder(mId, packetBufferTimePercent, vocalSampleRate)
  }

  override fun readSamples(samples: ShortArray, slientSizeArr: IntArray, extraSampleType: IntArray): Int {
    return readSamples(mId, samples, samples.size, slientSizeArr, extraSampleType)
  }

  /**
   * 由于允许在唱歌的时候可以调节伴奏音量并且要实时听到效果  这里就是设置音量的接口
   */
  override fun setAccompanyVolume(volume: Float, accompanyMax: Float) {
    setVolume(mId, volume, accompanyMax)
  }

  /**
   * 开启一个新的伴奏
   */
  override fun startAccompany(path: String) {
    start(mId, path)
  }

  /**
   * 暂停当前播放的伴奏
   */
  override fun pauseAccompany() {
    pause(mId)
  }

  /**
   * 继续播放当前的伴奏
   */
  override fun resumeAccompany() {
    resume(mId)
  }

  /**
   * 结束当前播放的伴奏
   */
  override fun stopAccompany() {
    stop(mId)
  }

  /**
   * 主播结束直播 关掉播放器
   */
  override fun destroy() {
    destroyDecoder(mId)
    release(mId)
    mId = 0
  }

  /**
   * 创建c++对象
   * @return 返回c++对象地址
   */
  private external fun create(): Long

  private external fun initDecoder(id: Long, packetBufferTimePercent: Float, vocalSampleRate: Int)
  private external fun readSamples(
    id: Long,
    samples: ShortArray,
    size: Int,
    slientSizeArr: IntArray,
    extraSampleType: IntArray
  ): Int

  private external fun setVolume(id: Long, volume: Float, accompanyMax: Float)
  private external fun start(id: Long, path: String)
  private external fun pause(id: Long)
  private external fun resume(id: Long)
  private external fun stop(id: Long)
  private external fun destroyDecoder(id: Long)
  private external fun release(id: Long)
}
