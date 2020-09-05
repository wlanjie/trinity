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

package com.trinity.sample.view

interface PasteManager {

  fun addPaste(path: String)

  fun addPasteWithStartTime(path: String, startTime: Long, duration: Long): PasteController

  fun addSubtitle(text: String, font: String): PasteController

  fun addSubtitleWithStartTime(text: String, font: String, startTime: Long, duration: Long): PasteController

  fun setDisplaySize(width: Int, height: Int)

  fun setOnPasteRestoreListener(l: (List<PasteController>) -> Unit)
}