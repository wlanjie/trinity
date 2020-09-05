package com.trinity.core

import android.content.Context
import com.trinity.editor.TrinityVideoEditor
import com.trinity.editor.TrinityVideoExport
import com.trinity.editor.VideoEditor
import com.trinity.editor.VideoExport

object TrinityCore {

  /**
   * 创建视频编辑实例
   * @param context Android上下文
   * @return 返回创建的视频编辑实例
   */
  fun createEditor(context: Context): TrinityVideoEditor {
    return VideoEditor(context)
  }

  fun createExport(context: Context): VideoExport {
    return TrinityVideoExport(context)
  }
}