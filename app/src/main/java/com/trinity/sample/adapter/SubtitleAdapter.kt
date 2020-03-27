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
package com.trinity.sample.adapter

import android.content.Context
import android.graphics.drawable.Drawable
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.ImageView
import androidx.core.content.res.ResourcesCompat.getFont
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R
import com.trinity.sample.editor.EditorPage
import com.trinity.sample.entity.PasterForm
import com.trinity.sample.entity.ResourceForm
import com.trinity.sample.entity.SubtitleInfo
import com.trinity.sample.text.FontForm
import de.hdodenhof.circleimageview.CircleImageView
import java.util.*
import java.util.concurrent.CopyOnWriteArrayList

class SubtitleAdapter(private val mContext: Context,
  private val onItemClickListener: (info: SubtitleInfo) -> Unit) : RecyclerView.Adapter<RecyclerView.ViewHolder?>(), View.OnClickListener {
  private var data = mutableListOf<PasterForm>()
  private val ids = ArrayList<Int>()
  private val fontData: CopyOnWriteArrayList<FontForm> = CopyOnWriteArrayList<FontForm>()
  private var mResourceForm: ResourceForm? = null
  private var mIsShowFont = false

  fun setData(data: ResourceForm?) {
    if (data?.pasterList == null) {
      return
    }
    mIsShowFont = false
    mResourceForm = data
    this.data.addAll(data.pasterList)
    notifyDataSetChanged()
  }

  fun clearData() {
    data.clear()
    fontData.clear()
    notifyDataSetChanged()
  }

  fun showFontData() {
    mIsShowFont = true;
    getFontFromLocal()
    notifyDataSetChanged()
  }

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
    val view: View = LayoutInflater.from(mContext).inflate(R.layout.item_subtitle_view, parent, false)
    val filterViewHolder = CaptionViewHolder(view)
    filterViewHolder.frameLayout = view.findViewById(R.id.resource_image)
    return filterViewHolder
  }

  override fun getItemViewType(position: Int): Int {
    return if (mIsShowFont) {
      FONT_TYPE
    } else {
      CAPTION_TYPE
    }
  }

  override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
    val captionViewHolder = holder as CaptionViewHolder
    var iconPath = ""
    if (getItemViewType(position) == CAPTION_TYPE) {
      iconPath = data[position].icon
    } else if (getItemViewType(position) == FONT_TYPE) {
      iconPath = fontData[position].icon
    }
    if (SYSTEM_FONT == iconPath) {
      //系统字体
      captionViewHolder.mImage.setImageResource(R.mipmap.ic_system_font_icon)
    } else {
//      ImageLoaderImpl().loadImage(mContext, iconPath)
//          .into(captionViewHolder.mImage, object : AbstractImageLoaderTarget<Drawable?>() {
//            fun onResourceReady(@NonNull resource: Drawable?) {
//              captionViewHolder.mImage.setImageDrawable(resource)
//            }
//          })
    }
    captionViewHolder.itemView.tag = holder
    captionViewHolder.itemView.setOnClickListener(this)
  }

  override fun getItemCount(): Int {
    return if (mIsShowFont) fontData.size else data.size
  }

  private class CaptionViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    var frameLayout: FrameLayout? = null
    val mImage: ImageView = itemView.findViewById(R.id.resource_image_view)
  }

  //添加系统字体
  private fun getFontFromLocal() {
//    val fileDownloaderModels: List<FileDownloaderModel> = DownloaderManager.getInstance().getDbController()
//        .getResourceByType(FONT_TYPE)
//    for (model in fileDownloaderModels) {
//      val fontForm = FontForm()
//      fontForm.level = model.getLevel()
//      fontForm.icon = model.getIcon()
//      fontForm.banner = model.getBanner()
//      fontForm.id = model.getId()
//      fontForm.md5 = model.getMd5()
//      fontForm.name = model.getName()
//      fontForm.type = model.getEffectType()
//      fontForm.url = model.getUrl()
//      fontForm.sort = model.getSort()
//      fontData.add(fontForm)
//      ids.add(fontForm.id)
//    }
    //添加系统字体
    val fontForm = FontForm()
    fontForm.icon = SYSTEM_FONT
    fontData.add(0, fontForm)
  }

//  private fun getFontByPaster(form: PasterForm): FontForm? {
//    var fontForm: FontForm? = null
//    for (form1 in fontData) {
//      if (form1.getId() === form.getFontId()) {
//        fontForm = form1
//        break
//      }
//    }
//    return fontForm
//  }

  override fun onClick(view: View) {
    val holder = view.tag as CaptionViewHolder
    val position: Int = holder.adapterPosition
    val type = getItemViewType(position)
    if (type == CAPTION_TYPE) {
      val form: PasterForm = data[position]
//      val path: String = DownloaderManager.getInstance().getDbController().getPathByUrl(form.getDownloadUrl())
//      if (path != null && !path.isEmpty()) {
//        if (mItemClick != null) {
//          val effectInfo = EffectInfo()
//          effectInfo.type = UIEditorPage.CAPTION
//          effectInfo.setPath(path)
//          val fontForm: FontForm? = getFontByPaster(form)
//          if (fontForm == null) {
//            effectInfo.fontPath = null
//          } else {
//            effectInfo.fontPath = DownloaderManager.getInstance().getDbController().getPathByUrl(fontForm.getUrl())
//          }
//          mItemClick.onItemClick(effectInfo, position)
//        }
//      } else {
//        downloadPaster(form, position)
//      }
    } else if (type == FONT_TYPE) {
      val form: FontForm = fontData[position]
      if (SYSTEM_FONT == form.icon) {
        //系统字体
        val subtitleInfo = SubtitleInfo()
        subtitleInfo.fontPath = SYSTEM_FONT
        subtitleInfo.type = EditorPage.FONT
        onItemClickListener(subtitleInfo)
      } else {
//        val path: String = DownloaderManager.getInstance().getDbController().getPathByUrl(form.getUrl())
//        if (path != null && !path.isEmpty()) {
//          if (mItemClick != null) {
//            val effectInfo = EffectInfo()
//            effectInfo.type = UIEditorPage.FONT
//            effectInfo.setPath(null)
//            effectInfo.fontPath = path
//            mItemClick.onItemClick(effectInfo, position)
//          }
//        } else {
//          downloadFont(form, position)
//        }
      }
    }
  }

  companion object {
    private const val CAPTION_TYPE = 6
    private const val FONT_TYPE = 1
    const val SYSTEM_FONT = "system_font"
  }
}