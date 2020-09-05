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

package com.trinity.sample.text

import android.graphics.Bitmap
import android.text.Layout

class TextBitmap {
  var mFontPath = ""
  var mText = ""
  var mTextColor = 0
  var mTextStrokeColor = 0
  var mBackgroundImg = ""
  var mBackgroundBmp: Bitmap? = null
  var mTextAlignment: Layout.Alignment? = null
  var mBmpWidth = 0
  var mBmpHeight = 0
  var mTextWidth = 0
  var mTextHeight = 0
  var mBackgroundColor = 0
  var mTextSize = 0
  var mTextPaddingX = 0
  var mTextPaddingY = 0
  var mMaxLines = 0

}