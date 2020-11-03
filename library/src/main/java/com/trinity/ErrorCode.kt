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

package com.trinity

/**
 * Create by wlanjie on 2019/4/15-下午8:36
 */
object ErrorCode {

  // 0成功
  const val SUCCESS = 0

  // 公共的 从-500开始
  const val FILE_NOT_FOUND = -500

  // Record -1000 开始

  // 正在录制中
  const val RECORDING = -1001
  const val FILE_NOT_EXISTS = -1002

  // 编辑模块 -2000开始
}