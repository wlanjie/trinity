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

import android.annotation.SuppressLint
import android.app.Dialog
import android.content.Context
import android.content.DialogInterface
import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.text.Editable
import android.text.Spanned
import android.text.TextUtils
import android.text.TextWatcher
import android.text.style.BackgroundColorSpan
import android.util.Log
import android.util.TypedValue
import android.view.*
import android.view.View.OnTouchListener
import android.view.inputmethod.InputMethodManager
import android.widget.*
import androidx.annotation.Nullable
import androidx.core.content.ContextCompat
import androidx.fragment.app.DialogFragment
import androidx.viewpager.widget.ViewPager
import com.google.android.material.tabs.TabLayout
import com.trinity.sample.R
import com.trinity.sample.adapter.ColorViewHolder
import com.trinity.sample.adapter.ColorViewPagerAdapter
import com.trinity.sample.view.WheelView
import java.io.File
import java.io.Serializable
import java.util.*

class TextDialog : DialogFragment() {

  companion object {
    private const val FONT_TYPE = 1
    private val ID_TITLE_ARRAY = intArrayOf(R.string.caption_effect_bottom_keyboard,
        R.string.caption_effect_bottom_color, R.string.caption_effect_bottom_font,
        R.string.caption_effect_bottom_animation
    )
    private val ID_ICON_ARRAY = intArrayOf(R.mipmap.ic_keyboard, R.mipmap.ic_text_color,
        R.mipmap.ic_font, R.mipmap.ic_effect
    )
    private const val NAME_SYSTEM_FONT = "系统字体"

    fun newInstance(editInfo: EditTextInfo, isInvert: Boolean): TextDialog {
      val dialog = TextDialog()
      val bundle = Bundle()
      bundle.putSerializable("edit", editInfo)
      bundle.putBoolean("invert", isInvert)
      dialog.arguments = bundle
      return dialog
    }

    /**
     * 处理十个字换行
     * @param text editText
     * @return 换行了的文字
     */
    fun lineFeedText(text: String): String? {
      val buffer = StringBuilder(text)
      val chars = text.toCharArray()
      var temp = 0
      var feedCount = 0
      var index = 0
      while (index <= chars.size) {
        if (temp >= 11) {
          buffer.insert(index + feedCount - 1, 10.toChar())
          feedCount++
          temp = 1
        }
        if (index == chars.size) {
          break
        }
        if (chars[index].toInt() != 10) {
          temp++
        } else {
          temp = 0
        }
        index++
      }
      return buffer.toString()
    }
  }

  private var mEditView: AutoResizingEditText ?= null
  private var mConfirm: View? = null
  private var textLimit: TextView? = null
  private var mFormList = mutableListOf<FontForm>()
  private var mFontDataList = mutableListOf<WheelView.DataModel>()
  private var mAnimationSelectPosition = 0
  private var fontList: GridView? = null
  private var fontAdapter: FontAdapter? = null
  private var pageContainer: FrameLayout? = null
  private var mEditInfo: EditTextInfo? = null
  private var mSelectedIndex = 0
  private var mOnStateChangeListener: OnStateChangeListener? = null
  private var mBack: View? = null
  private var mTabLayout: TabLayout? = null
  private var mPagerViewStack = mutableListOf<View>()
  private var mWva: WheelView? = null
  private var sIsShowing = false
  private var mUseInvert = false
  private var masks = mutableListOf<BackgroundColorSpan>()
  private val textWatch = MyTextWatcher(this)

  override fun onDismiss(dialog: DialogInterface) {
    super.onDismiss(dialog)
    mEditView?.removeTextChangedListener(textWatch)
    callbackResult()
    sIsShowing = false
  }

  override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
    val dialog = super.onCreateDialog(savedInstanceState)
    dialog.setOnKeyListener(DialogInterface.OnKeyListener { _, keyCode, _ ->
      if (keyCode == KeyEvent.KEYCODE_BACK) {
        dialog.dismiss()
        return@OnKeyListener true
      }
      false
    })
    dialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)
    return dialog
  }

  @SuppressLint("SetTextI18n")
  override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
    val contentView = View.inflate(activity, R.layout.row_text_bottom, null)
    mEditInfo = arguments?.getSerializable("edit") as EditTextInfo?
    mUseInvert = arguments?.getBoolean("invert") ?: false
    if (mEditInfo == null) {
      dismiss()
      return contentView
    }
    initTableView(contentView)
    mEditView = contentView.findViewById(R.id.content_text)
    mConfirm = contentView.findViewById(R.id.iv_confirm)
    mBack = contentView.findViewById(R.id.iv_back)
    mEditView?.setTextOnly(mEditInfo?.isTextOnly ?: false)
    mEditView?.setText(mEditInfo?.text ?: "")
    mEditView?.setFontPath(mEditInfo?.font ?: "")
    mEditView?.setCurrentColor(mEditInfo?.textColor ?: Color.WHITE)
    mEditView?.setTextStrokeColor(mEditInfo?.textStrokeColor ?: Color.WHITE)
    val frameLayout = contentView.findViewById<View>(R.id.fl_editText) as FrameLayout
    val layoutParams = frameLayout.layoutParams
    layoutParams.width = mEditInfo?.layoutWidth ?: 0
    frameLayout.layoutParams = layoutParams
    if (mEditInfo?.isTextOnly == false) {
      mEditView?.setTextWidth(mEditInfo?.textWidth ?: 0)
      mEditView?.setTextHeight(mEditInfo?.textHeight ?: 0)
    }
    mEditView?.textSize = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 7f, resources.displayMetrics)
    pageContainer = contentView.findViewById<View>(R.id.container) as FrameLayout
    mPagerViewStack.add(View(activity))
    mPagerViewStack.add(contentView.findViewById(R.id.color_container))
    mPagerViewStack.add(contentView.findViewById(R.id.font_layout_new))
    mPagerViewStack.add(contentView.findViewById(R.id.font_animation))
    initWLView(contentView)
    val colorTabHost = contentView.findViewById<View>(R.id.color_tab_host) as TabLayout
    val colorViewPager: ViewPager = contentView.findViewById<View>(R.id.color_viewpager) as ViewPager
    val colorPagerAdapter = ColorViewPagerAdapter()
    initFontAnimationView(contentView)
    val colorHolder = ColorViewHolder(contentView.context,
        getString(R.string.subtitle_text_color), false, mEditInfo?.dTextColor ?: Color.WHITE)
    val strokeHolder = ColorViewHolder(contentView.context,
        getString(R.string.subtitle_text_stroke), true, mEditInfo?.dTextStrokeColor ?: Color.WHITE)

    colorHolder.setItemClickListener(object : ColorViewHolder.OnItemClickListener {
      override fun onItemClick(item: ColorViewHolder.ColorItem?) {
        mEditView?.setCurrentColor(item?.color ?: Color.WHITE)
      }
    })
    strokeHolder.setItemClickListener(object : ColorViewHolder.OnItemClickListener {
      override fun onItemClick(item: ColorViewHolder.ColorItem?) {
        mEditView?.setTextStrokeColor(item?.strokeColor ?: Color.WHITE)
      }
    })
    colorPagerAdapter.addViewHolder(colorHolder)
    colorPagerAdapter.addViewHolder(strokeHolder)
    colorViewPager.adapter = colorPagerAdapter
    colorTabHost.setupWithViewPager(colorViewPager)
//    fontList = contentView.findViewById(R.id.font_list)
//    fontAdapter = FontAdapter(fontList)
//    fontAdapter?.data = getFontList()
//    fontList?.adapter = fontAdapter
//    fontList?.setItemChecked(mSelectedIndex, true)
//    fontList?.onItemClickListener = AdapterView.OnItemClickListener { parent, _, position, _ ->
//      val font: FontForm = parent.getItemAtPosition(position) as FontForm
//      val path: String = font.url
//      var f = File(path)
//      var ifd = f.exists() && f.isDirectory
//      if (!ifd) {
//        f = File(path + "tmp")
//        ifd = f.exists() && f.isDirectory
//      }
//      if (ifd) {
//        val fds = f.list { _, name -> name.toLowerCase(Locale.ROOT).endsWith(".ttf") }
//        if (fds.isNotEmpty()) {
//          mEditView?.setFontPath(File(f, fds[0]).absolutePath)
//        }
//      }
//      fontList?.setItemChecked(position, true)
//    }
    val localLayoutParams = dialog?.window?.attributes
    localLayoutParams?.gravity = Gravity.BOTTOM
    localLayoutParams?.width = WindowManager.LayoutParams.MATCH_PARENT
    setOnClick()
    textLimit = contentView.findViewById(R.id.message)
    requestFocusForKeyboard()
    if (isTextOnly()) {
      textLimit?.visibility = View.GONE
    } else {
      val text: CharSequence = mEditView?.text ?: ""
      if (TextUtils.isEmpty(text)) {
        textLimit?.text = "0 / 10"
      } else {
        textLimit?.text = count(text.toString()).toString() + " / 10"
      }
    }
    return contentView
  }

  private fun initWLView(contentView: View) {
//    mFormList = getFontList()
//    if (mFormList == null) {
//      return
//    }
//    for (fontForm in mFormList) {
//      val fontDateBean = FontDateBean(fontForm.name)
//      mFontDataList.add(fontDateBean)
//    }
    mWva = contentView.findViewById(R.id.font_custom_view)
    mWva?.setOffset(2)
    mWva?.setItems(mFontDataList)
    mWva?.setOnWheelViewListener(object : WheelView.OnWheelViewListener {
      override fun onSelected(selectedIndex: Int, item: String?) {
        if (selectedIndex == 0) {
          //系统字体
          mEditView?.setTypeface(Typeface.DEFAULT)
        } else {
          //非系统字体
          val font: FontForm = mFormList[selectedIndex]
          val path: String = font.url
          var f = File(path)
          var ifd = f.exists() && f.isDirectory
          if (!ifd) {
            f = File(path + "tmp")
            ifd = f.exists() && f.isDirectory
          }
          if (ifd) {
            val fds = f.list { _, name -> name.toLowerCase(Locale.ROOT).endsWith(".ttf") }
            if (fds?.isNotEmpty() == true) {
              mEditView?.setFontPath(File(f, fds[0]).absolutePath)
            }
          }
        }
        Log.d("TAG", "selectedIndex: $selectedIndex, item: $item")
      }
    })
  }

  /**
   * 初始化字体动画View
   *
   * @param contentView 布局根容器
   */
  private fun initFontAnimationView(contentView: View) {
//    val recyclerView: RecyclerView = contentView.findViewById(R.id.font_animation_view)
//    val fontAnimationAdapter = FontAnimationAdapter(contentView.context)
//    fontAnimationAdapter.setSelectPosition(mEditInfo!!.mAnimationSelect)
//    fontAnimationAdapter.setOnItemClickListener(mOnItemClickListener)
//    val linearLayoutManager = LinearLayoutManager(contentView.context)
//    linearLayoutManager.orientation = LinearLayoutManager.HORIZONTAL
//    recyclerView.layoutManager = linearLayoutManager
//    recyclerView.addItemDecoration(object: RecyclerView.ItemDecoration() {
//      override fun getItemOffsets(outRect: Rect, view: View, parent: RecyclerView, state: RecyclerView.State) {
//        outRect.left = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 5f, resources.displayMetrics).toInt()
//        outRect.right = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 5f, resources.displayMetrics).toInt()
//      }
//    })
//    recyclerView.adapter = fontAnimationAdapter
  }

  private val mOnItemClickListener: OnItemClickListener = object : OnItemClickListener {
    override fun onItemClick(animPosition: Int) {
      if (mUseInvert) {
        return
      }
      mAnimationSelectPosition = animPosition
//      when (animPosition) {
//        EFFECT_NONE -> mActionBase = null
//        EFFECT_UP -> {
//          mActionBase = ActionTranslate()
//          (mActionBase as ActionTranslate?).setToPointY(1f)
//        }
//        EFFECT_RIGHT -> {
//          mActionBase = ActionTranslate()
//          (mActionBase as ActionTranslate?).setToPointX(1f)
//        }
//        EFFECT_LEFT -> {
//          mActionBase = ActionTranslate()
//          (mActionBase as ActionTranslate?).setToPointX(-1f)
//        }
//        EFFECT_DOWN -> {
//          mActionBase = ActionTranslate()
//          (mActionBase as ActionTranslate?).setToPointY(-1f)
//        }
//        EFFECT_SCALE -> {
//          mActionBase = ActionScale()
//          (mActionBase as ActionScale?).setFromScale(1f)
//          (mActionBase as ActionScale?).setToScale(0.25f)
//        }
//        EFFECT_LINEARWIPE -> {
//          mActionBase = ActionWipe()
//          (mActionBase as ActionWipe?).setWipeMode(ActionWipe.WIPE_MODE_DISAPPEAR)
//          (mActionBase as ActionWipe?).setDirection(ActionWipe.DIRECTION_RIGHT)
//        }
//        EFFECT_FADE -> {
//          mActionBase = ActionFade()
//          (mActionBase as ActionFade?).setFromAlpha(1.0f)
//          (mActionBase as ActionFade?).setToAlpha(0.2f)
//        }
//        else -> {
//        }
//      }
    }
  }

  /**
   * 初始化新View
   * @param contentView 内容View
   */
  private fun initTableView(contentView: View) {
    mTabLayout = contentView.findViewById(R.id.tl_tab)
    mTabLayout?.tabMode = TabLayout.MODE_SCROLLABLE
    var length = ID_TITLE_ARRAY.size
    if (mEditInfo?.isTextOnly == false) {
      length -= 1
    }
    for (i in 0 until length) {
      val item: View = LayoutInflater.from(contentView.context).inflate(R.layout.item_text_bottom_item, contentView as ViewGroup, false)
      (item.findViewById<ImageView>(R.id.iv_icon)).setImageResource(ID_ICON_ARRAY[i])
      (item.findViewById<TextView>(R.id.tv_title)).setText(ID_TITLE_ARRAY[i])
      mTabLayout?.let {
        it.addTab(it.newTab().setCustomView(item))
      }
    }
    mTabLayout?.setSelectedTabIndicatorColor(ContextCompat.getColor(contentView.context, R.color.colorPrimary))
    mTabLayout?.addOnTabSelectedListener(mOnTabSelectedListener)
  }

  /**
   * TabLayout的点击监听
   */
  private val mOnTabSelectedListener: TabLayout.OnTabSelectedListener = object : TabLayout.OnTabSelectedListener {
    override fun onTabSelected(tab: TabLayout.Tab) {
      val checkedIndex = tab.position
      if (checkedIndex == 0) {
        pageContainer?.visibility = View.GONE
        mEditView?.isEnabled = true
        openKeyboard()
      } else {
        pageContainer?.visibility = View.VISIBLE
        closeKeyboard()
        mPagerViewStack[checkedIndex].visibility = View.VISIBLE
        if (checkedIndex == 1) {
          if (fontList?.checkedItemPosition == -1) {
            val position = (fontList?.adapter as FontAdapter).getLastCheckedPosition(null)
            fontList?.setItemChecked(position, true)
            fontList?.smoothScrollToPosition(position)
          }
        } else {
          mEditView?.isEnabled = false
        }
        if (checkedIndex == 2 && mWva != null) {
          mWva?.setSelection(mSelectedIndex)
        }
      }
    }

    override fun onTabUnselected(tab: TabLayout.Tab?) {}
    override fun onTabReselected(tab: TabLayout.Tab?) {}
  }

  override fun onViewCreated(view: View, @Nullable savedInstanceState: Bundle?) {
    super.onViewCreated(view, savedInstanceState)
    openKeyboard()
  }

  private fun getFontList(): MutableList<FontForm>? {
    var index = 0
    val fonts: MutableList<FontForm> = ArrayList<FontForm>()
    val fontForm = FontForm()
    fontForm.name = NAME_SYSTEM_FONT
    fonts.add(fontForm)
//    for (m in list) {
//      val font = FontForm()
//      font.id = m.getId()
//      font.name = m.getName()
//      font.banner = m.getBanner()
//      font.url = m.getPath()
//      fonts.add(font)
//      if (m.getPath().toString() + "/font.ttf" == mEditInfo!!.font) {
//        mSelectedIndex = index
//      }
//      index++
//    }
    return fonts
  }

  override fun onStart() {
    super.onStart()
  }

  override fun onStop() {
    super.onStop()
    closeKeyboard()
  }

  private fun filterComposingText(s: Editable): String {
    val sb = StringBuilder()
    var composingStart = 0
    var composingEnd = 0
    val sps = s.getSpans(0, s.length, Any::class.java)
    if (sps != null) {
      for (i in sps.indices.reversed()) {
        val o = sps[i]
        val fl = s.getSpanFlags(o)
        if (fl and Spanned.SPAN_COMPOSING != 0) {
          composingStart = s.getSpanStart(o)
          composingEnd = s.getSpanEnd(o)
          break
        }
      }
    }
    sb.append(s.subSequence(0, composingStart))
    sb.append(s.subSequence(composingEnd, s.length))
    if (composingStart == composingEnd) {
      if (masks.size > 0) {
        for (mask in masks) {
          s.removeSpan(mask)
        }
        masks.clear()
      }
    } else {
      val mask = BackgroundColorSpan(resources.getColor(R.color.accent_material_dark))
      s.setSpan(mask, composingStart, composingEnd, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
      masks.add(mask)
    }
    return sb.toString()
  }

  private fun count(text: String): Int {
    val len = text.length
    var skip: Int
    var letter = 0
    var chinese = 0
    var i = 0
    while (i < len) {
      val code = text.codePointAt(i)
      skip = Character.charCount(code)
      if (code == 10) {
        i += skip
        continue
      }
      val s = text.substring(i, i + skip)
      if (isChinese(s)) {
        chinese++
      } else {
        letter++
      }
      i += skip
    }
    letter = if (letter % 2 == 0) letter / 2 else letter / 2 + 1
    return chinese + letter
  }

  // 完整的判断中文汉字和符号
  private fun isChinese(strName: String): Boolean {
    val ch = strName.toCharArray()
    for (i in ch.indices) {
      val c = ch[i]
      if (isChinese(c)) {
        return true
      }
    }
    return false
  }

  private fun isChinese(c: Char): Boolean {
    val ub = Character.UnicodeBlock.of(c)
    return ub === Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS || ub === Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS || ub === Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A || ub === Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B || ub === Character.UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION || ub === Character.UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS || ub === Character.UnicodeBlock.GENERAL_PUNCTUATION
  }

  private class MyTextWatcher(
      private val dialog: TextDialog
  ) : TextWatcher {
    private var text: String? = null
    private var editStart = 0
    private var editEnd = 0
    private var toastOutOf: Toast? = null

    private fun showOutofCount(context: Context, text: String, gravity: Int, xOffset: Int, yOffset: Int, duration: Int) {
      toastOutOf?.cancel()
      toastOutOf = null
      toastOutOf = Toast.makeText(context, text, duration)
      toastOutOf?.setGravity(gravity, xOffset, yOffset)
      toastOutOf?.show()
    }

    override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {}

    override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}

    override fun afterTextChanged(s: Editable) {
      text = dialog.filterComposingText(s)
      val count: Int = dialog.count(text ?: "")
      var maxLength = 0
      maxLength = if (!dialog.isTextOnly()) {
        dialog.textLimit?.text = (if (count > 10) 10 else count).toString() + " / 10"
        10
      } else {
        //纯字幕限制90个字
        90
      }
      editStart = dialog.mEditView?.selectionStart ?: 0
      editEnd = dialog.mEditView?.selectionEnd ?: 0
      // 限定EditText只能输入maxLength个数字
      if (count > maxLength && editStart > 0) {
        // 默认光标在最前端，所以当输入第11个数字的时候，删掉（光标位置从11-1到11）的数字，这样就无法输入超过10个以后的数字
        dialog.activity?.let {
          showOutofCount(it, it.getString(R.string.text_count_outof) ?: "", Gravity.CENTER, 0, 0,
              Toast.LENGTH_SHORT)
        }
        s.delete(editStart - 1, editEnd)
        dialog.mEditView?.text = s
        dialog.mEditView?.setSelection(s.length)
      }
      val s1: String = lineFeedText(s.toString()) ?: ""
      if (s1 != s.toString()) {
        dialog.mEditView?.setText(s1)
        if (dialog.mEditView?.text?.length ?: 0 >= s1.length) {
          //适配部分机型的角标越界问题
          dialog.mEditView?.setSelection(s1.length)
        } else {
          dialog.mEditView?.setSelection(dialog.mEditView?.text?.length ?: 0)
        }
      }
    }

  }

  private fun isTextOnly(): Boolean {
    return mEditInfo?.isTextOnly ?: false
  }

  @SuppressLint("ClickableViewAccessibility")
  private fun setOnClick() {
    mEditView?.addTextChangedListener(textWatch)
    mEditView?.setOnTouchListener(OnTouchListener { _, event ->
      if (mTabLayout?.selectedTabPosition != 0 && event.action == MotionEvent.ACTION_DOWN) {
        val tabAt = mTabLayout?.getTabAt(0)
        if (tabAt != null) {
          tabAt.select()
          return@OnTouchListener true
        }
      }
      false
    })
    mBack?.setOnClickListener { dismiss() }
    mConfirm?.setOnClickListener {
//      mEditInfo?.mAnimation = mActionBase
//      mEditInfo?.mAnimationSelect = mAnimationSelectPosition
      mEditView?.removeTextChangedListener(textWatch)
      val editable: Editable? = mEditView?.text
      editable?.let {
        val comment = filterComposingText(it)
      }
      callbackResult()
      dismiss()
    }
  }

  private fun callbackResult() {
    if (mOnStateChangeListener != null) {
      val text: CharSequence = mEditView?.text?.toString() ?: ""
      mEditInfo?.text = if (TextUtils.isEmpty(text)) "" else text.toString()
      if (TextUtils.isEmpty(text)) {
        mEditInfo?.text = ""
      } else {
        mEditInfo?.text = lineFeedText(text.toString()) ?: ""
      }
      mEditInfo?.textColor = mEditView?.currentTextColor ?: Color.WHITE
      mEditInfo?.textStrokeColor = mEditView?.getTextStrokeColor() ?: Color.WHITE
      mEditInfo?.font = mEditView?.getFontPath() ?: ""
      mEditInfo?.textWidth = mEditView?.width ?: 0
      mEditInfo?.textHeight = mEditView?.height ?: 0
      mOnStateChangeListener?.onTextEditCompleted(mEditInfo)
    }
  }

  fun closeKeyboard() {
    val inputManager = activity?.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
    if (inputManager.isActive) {
      inputManager.hideSoftInputFromWindow(mEditView?.windowToken, 0)
    }
  }

  fun openKeyboard() {
    mEditView?.postDelayed(mOpenKeyboardRunnable, 300)
  }

  private val mOpenKeyboardRunnable = Runnable {
    if (activity == null) {
      return@Runnable
    }
    try {
      requestFocusForKeyboard()
      val input = activity?.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
      if (!input.showSoftInput(mEditView, 0)) {
        openKeyboard()
      } else {
        mEditView?.setSelection(mEditView?.text?.length ?: 0)
      }
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  private fun requestFocusForKeyboard() {
    mEditView?.isFocusable = true
    mEditView?.isFocusableInTouchMode = true
    mEditView?.requestFocus()
    mEditView?.requestFocusFromTouch()
  }

  override fun onCreate(paramBundle: Bundle?) {
    super.onCreate(paramBundle)
    setStyle(STYLE_NO_FRAME, R.style.TextDlgStyle)
  }

  override fun onResume() {
    dialog?.window?.setLayout(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT)
    super.onResume()
  }

  fun setOnStateChangeListener(onStateChangeListener: OnStateChangeListener?) {
    mOnStateChangeListener = onStateChangeListener
  }

  interface OnStateChangeListener {
    fun onTextEditCompleted(result: EditTextInfo?)
  }

  class FontAdapter(
      private val fontListView: GridView
  ) : BaseAdapter() {
    private val list: MutableList<FontForm> = ArrayList<FontForm>()

    fun getLastCheckedPosition(path: String?): Int {
      return 0
    }

    var data: MutableList<FontForm>
      get() = list
      set(data) {
        if (data.isEmpty()) {
          return
        }
        list.addAll(data)
        notifyDataSetChanged()
      }

    override fun getCount(): Int {
      return list.size
    }

    override fun getItem(position: Int): FontForm {
      return list[position]
    }

    override fun getItemId(position: Int): Long {
      return 0
    }

    override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
      var view = convertView
      val localViewHolder: FontItemViewMediator
      if (view == null) {
        localViewHolder = FontItemViewMediator(parent)
        view = localViewHolder.view
        //Log.d("share_menu", "分享菜单的position：" + paramInt);
      } else {
        localViewHolder = convertView?.tag as FontItemViewMediator
      }
      val item: FontForm = getItem(position)
      localViewHolder.setData(item)
      if (fontListView.checkedItemPosition == position) {
        localViewHolder.setSelected(true)
      } else {
        localViewHolder.setSelected(false)
      }
      return view
    }
  }

  private class FontItemViewMediator(parent: ViewGroup) {
    private val image: ImageView
    private val select: View
    private val indiator: ImageView
    private val name: TextView
    val view: View
    private var fontInfo: FontForm? = null
    fun setData(font: FontForm) {
      fontInfo = font
      name.text = font.name
//      ImageLoaderImpl().loadImage(getActivity(), font.getBanner()).into(image)
    }

    fun setSelected(selected: Boolean) {
      select.visibility = if (selected) View.VISIBLE else View.GONE
    }

    init {
      view = View.inflate(parent.context, R.layout.item_font_effect, null)
      select = view.findViewById(R.id.selected)
      image = view.findViewById(R.id.font_item_image)
      indiator = view.findViewById(R.id.indiator)
      name = view.findViewById(R.id.item_name)
      view.tag = this
    }
  }

  interface OnItemClickListener {
    fun onItemClick(animPosition: Int)
  }

  class FontDateBean(private val text: String) : WheelView.DataModel {

    override fun getText(): String {
      return text
    }

  }

  class EditTextInfo : Serializable {
    var text = ""
    var textStrokeColor = Color.TRANSPARENT
    var font = ""
    var textColor = Color.WHITE
    var textWidth = 0
    var textHeight = 0
    var dTextColor = Color.WHITE
    var dTextStrokeColor = Color.WHITE
    var isTextOnly = false
    var layoutWidth = 0
    var animationSelect = 0
  }
}