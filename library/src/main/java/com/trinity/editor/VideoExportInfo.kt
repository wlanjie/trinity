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

package com.trinity.editor

class VideoExportInfo(val path: String) {
    var width = 544
    var height = 960
    var frameRate = 25
    var videoBitRate = 2000
    var sampleRate = 44100
    var channelCount = 1
    var audioBitRate = 128
    var mediaCodecDecode = true
    var mediaCodecEncode = true
}