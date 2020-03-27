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

interface PasteController {
    fun setPasteView(baseView: PasteBaseView)

    fun getText(): String?

    fun getTextColor(): Int

    fun getConfigTextColor(): Int

    fun getConfigTextStrokeColor(): Int

    fun getTextStrokeColor(): Int

    fun isTextHasStroke(): Boolean

    fun getTextBgLabelColor(): Int

    fun getPasteCenterY(): Int

    fun getPasteCenterX(): Int

    fun getPasteWidth(): Int

    fun getPasteHeight(): Int

    fun getPasteRotation(): Float

    fun getPasteTextOffsetX(): Int

    fun getPasteTextOffsetY(): Int

    fun getPasteTextWidth(): Int

    fun getPasteTextHeight(): Int

    fun getPasteTextRotation(): Float

    fun getPasteTextFont(): String?

    fun editStart()

    fun editCompleted(): Int

    fun removePaste(): Int

    fun isPasteExists(): Boolean

    fun setPasteStartTime(start: Long)

    fun setPasteDuration(duration: Long)

    fun getPasteStartTime(): Long

    fun getPasteDuration(): Long

    fun getPasteIconPath(): String?

//    fun createPasterlayer(var1: TextureView?): AnimPlayerView?

    fun getPasteType(): Int

    fun isPasteMirrored(): Boolean

//    fun getEffect(): EffectBase?

//    fun setEffect(var1: EffectBase?)

    fun setOnlyApplyUI(var1: Boolean)

    fun isOnlyApplyUI(): Boolean

    fun setRevert(var1: Boolean)

    fun isRevert(): Boolean
}