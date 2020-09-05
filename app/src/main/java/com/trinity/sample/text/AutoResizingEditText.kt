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

import android.content.Context
import android.text.Editable
import android.text.Selection
import android.text.TextUtils.TruncateAt
import android.text.method.ArrowKeyMovementMethod
import android.text.method.MovementMethod
import android.util.AttributeSet
import android.view.View
import android.widget.EditText

class AutoResizingEditText : AutoResizingTextView {

  constructor(context: Context) : super(context) {
    setLayerType(View.LAYER_TYPE_SOFTWARE, paint)
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    setLayerType(View.LAYER_TYPE_SOFTWARE, paint)
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    setLayerType(View.LAYER_TYPE_SOFTWARE, paint)
  }


  override fun getDefaultEditable(): Boolean {
    return true
  }

  override fun getDefaultMovementMethod(): MovementMethod? {
    return ArrowKeyMovementMethod.getInstance()
  }

  override fun getText(): Editable? {
    return super.getText() as Editable
  }

  override fun setText(text: CharSequence, type: BufferType?) {
    super.setText(text, BufferType.EDITABLE)
  }

  /**
   * Convenience for [Selection.setSelection].
   */
  fun setSelection(start: Int, stop: Int) {
    Selection.setSelection(text, start, stop)
  }

  /**
   * Convenience for [Selection.setSelection].
   */
  fun setSelection(index: Int) {
    Selection.setSelection(text, index)
  }

  /**
   * Convenience for [Selection.selectAll].
   */
  fun selectAll() {
    Selection.selectAll(text)
  }

  /**
   * Convenience for [Selection.extendSelection].
   */
  fun extendSelection(index: Int) {
    Selection.extendSelection(text, index)
  }

  override fun setEllipsize(ellipsis: TruncateAt) {
    require(ellipsis != TruncateAt.MARQUEE) {
      ("EditText cannot use the ellipsize mode "
          + "TextUtils.TruncateAt.MARQUEE")
    }
    super.setEllipsize(ellipsis)
  }

  override fun getAccessibilityClassName(): CharSequence? {
    return EditText::class.java.name
  }

}