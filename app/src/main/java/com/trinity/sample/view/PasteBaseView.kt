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

import android.graphics.Bitmap
import android.text.Layout
import android.view.View

interface PasteBaseView {
    fun getTextMaxLines(): Int

    fun getTextAlign(): Layout.Alignment?

    fun getTextPaddingX(): Int

    fun getTextPaddingY(): Int

    fun getTextFixSize(): Int

    fun getBackgroundBitmap(): Bitmap?

    fun getText(): String?

    fun getTextColor(): Int

    fun getTextStrokeColor(): Int

    fun isTextHasStroke(): Boolean

    fun isTextHasLabel(): Boolean

    fun getTextBgLabelColor(): Int

    fun getPasteTextOffsetX(): Int

    fun getPasteTextOffsetY(): Int

    fun getPasteTextWidth(): Int

    fun getPasteTextHeight(): Int

    fun getPasteTextRotation(): Float

    fun getPasteTextFont(): String?

    fun getPasteWidth(): Int

    fun getPasteHeight(): Int

    fun getPasteCenterY(): Int

    fun getPasteCenterX(): Int

    fun getPasteRotation(): Float

    fun transToImage(): Bitmap?

    fun getPasteView(): View?

    fun getTextView(): View?

    fun isPasteMirrored(): Boolean
}