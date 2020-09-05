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

@file:Suppress("DEPRECATION")

package com.trinity.sample.text

import android.annotation.TargetApi
import android.content.Context
import android.graphics.*
import android.graphics.Paint.Join
import android.os.Build.VERSION
import android.os.Build.VERSION_CODES
import android.text.Layout
import android.text.StaticLayout
import android.text.TextPaint
import android.text.TextUtils
import android.util.AttributeSet
import android.util.Log
import android.util.TypedValue
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.widget.AppCompatTextView
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import kotlin.math.max

open class AutoResizingTextView : AppCompatTextView {

  companion object {
    private const val NO_LINE_LIMIT = -1
  }

  private var mCurrentColor = Color.WHITE
  private var mCurrentStrokeColor = Color.WHITE
  protected var mStrokeWidth = 0f
  protected var mStrokeJoin: Join? = null
  protected var mStrokeMiter = 0f
  private var mFrozen = false
  var mScaleByDrawable = false
  private val mTextRect = RectF()
  private var mAvailableSpaceRect: RectF? = null
  private var mPaint: TextPaint? = null
  private var mTextAngle = 0f
  private var mMaxTextSize = 0f
  private var mSpacingMult = 1.0f
  private var mSpacingAdd = 0.0f
  private var mMinTextSize = 0f
  private var mLastWidth = 0
  private var mLastHeight = 0
  private var mWidth = 0
  private var mHeight = 0
  private var mTop = 0
  private var mLeft = 0
  private var mRight = 0
  private var mBottom = 0
  private var mFontPath: String? = null
  private var isEditCompleted = false
  private var isMirror = false
  private var isTextOnly = false
  private var mLayout: StaticLayout? = null
  private var mBitmapGenerator: TextBitmapGenerator? = null
  private var mTextBitmap: TextBitmap? = null
  private var mMaxLines = 0
  private var mInitialized = false
  private var mMaxWidthWhenOutof = 0
  private var mMaxHeightWhenOutof = 0
  private var mWidthLimit = 0

  constructor(context: Context) : super(context) {
    init()
  }

  constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
    init()
  }

  constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
    init()
  }

  @TargetApi(VERSION_CODES.JELLY_BEAN)
  fun generateSpacingmultAndSpacingadd(args: FloatArray, textview: TextView) {
    if (VERSION.SDK_INT >= VERSION_CODES.JELLY_BEAN) {
      args[0] = textview.lineSpacingMultiplier
      args[1] = textview.lineSpacingExtra
    } else {
      args[0] = 1f
      args[1] = 0f
    }
  }

  private fun init() {
    mStrokeJoin = Paint.Join.ROUND
    mStrokeMiter = 10f
    mMaxTextSize = TypedValue.applyDimension(
        TypedValue.COMPLEX_UNIT_SP, 180f,
        resources.displayMetrics)
    mMaxTextSize = resources.displayMetrics.widthPixels.toFloat()
    mAvailableSpaceRect = RectF()
    if (mMaxLines == 0) {
      // no value was assigned during construction
      mMaxLines = NO_LINE_LIMIT
    }
    val addAndMult = FloatArray(2)
    generateSpacingmultAndSpacingadd(addAndMult, this)
    mSpacingMult = addAndMult[0]
    mSpacingAdd = addAndMult[1]
    mInitialized = true

    post {
      val pw = (parent as ViewGroup).width
      val ph = (parent as ViewGroup).height
      val padding = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 30f, resources.displayMetrics).toInt()
      mMaxWidthWhenOutof = pw - padding
      mMaxHeightWhenOutof = ph - padding * 3
      maxWidth = mMaxWidthWhenOutof
      maxHeight = mMaxHeightWhenOutof
    }
  }

  private val mSizeTester: SizeTester = object : SizeTester {
    @TargetApi(VERSION_CODES.JELLY_BEAN)

    override fun onTestSize(suggestedSize: Int, availableSPace: RectF?, text: String?): Int {
      mPaint?.textSize = suggestedSize.toFloat()
      mLayout = StaticLayout(text, mPaint,
          mWidthLimit, Layout.Alignment.ALIGN_NORMAL, mSpacingMult,
          mSpacingAdd, true)
      if (mMaxLines != NO_LINE_LIMIT
          && (mLayout?.lineCount ?: 0 > mMaxLines || lineCount > mMaxLines)) {
        //大了
        Log.d("suggest", "mMaxLines return  ,maxLine : $mMaxLines")
        return 1
      }
      val tl = calculateMaxLinesByText(text ?: "")
      if (mLayout?.lineCount ?: 0 > tl) {
        Log.d("trinity", "mLayout return ,tl : $tl")
        return 1
      }
      mTextRect.bottom = mLayout?.height?.toFloat() ?: 0f
      mTextRect.right = getLayoutMaxWidth().toFloat()
      //            }
      mTextRect.offsetTo(0f, 0f)
      Log.d("trinity", "suggest size : " + suggestedSize +
          " width : " + mTextRect.right +
          " height : " + mTextRect.bottom +
          " match : " + availableSPace?.contains(mTextRect) +
          " line : " + lineCount +
          " maxLine : " + maxLines +
          " layoutLine : " + (mLayout?.lineCount ?: 0)
      )
      return if (availableSPace?.contains(mTextRect) == true) {
        // may be too small, don't worry we will find the best match
        -1
      } else {
        // too big
        1
      }
    }
  }

  private var mImageView: ImageView? = null

  fun setImageView(imageView: ImageView?) {
    mImageView = imageView
  }

  fun layoutToBitmap(): Bitmap? {
    val textWidth = mWidth
    val textHeight = mHeight
    var layout = layout
    if (layout == null) {
      measureSelf(mWidth, mHeight)
      val w = mWidth + paddingLeft + paddingRight
      val h = mHeight + paddingTop + paddingBottom
      super.measure(MeasureSpec.makeMeasureSpec(w, MeasureSpec.EXACTLY),
          MeasureSpec.makeMeasureSpec(h, MeasureSpec.EXACTLY))
      layout = getLayout()
    }
    if (layout == null) {
      layout = mLayout
      if (layout == null) {
        return null
      }
    }
    var dw = width
    var dh = height
    val rw = layout.width
    val rh = layout.height
    var firstInit = false
    if (dw == 0 || dh == 0) {
      dw = mWidth
      dh = mHeight
      firstInit = true
    }
    val w: Int
    val h: Int
    if (isTextOnly) {
      w = dw
      h = dh
    } else {
      if (firstInit) {
        w = dw
        h = dh
      } else {
        w = dw - paddingLeft - paddingRight
        h = dh - paddingTop - paddingBottom
      }
    }
    if (w == 0 || h == 0) {
      return null
    }
    val textBmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
    val canvas = Canvas(textBmp)
    canvas.translate((w - rw) / 2.toFloat(), (h - rh) / 2.toFloat())
    val paint = layout.paint
    if (mCurrentStrokeColor != 0) {
      paint.style = Paint.Style.STROKE
      paint.strokeJoin = mStrokeJoin
      paint.strokeMiter = mStrokeMiter
      paint.strokeWidth = generateStrokeWidth()
      paint.color = mCurrentStrokeColor
      layout.draw(canvas)
    }
    paint.color = mCurrentColor
    paint.style = Paint.Style.FILL
    layout.draw(canvas)
    if (mImageView != null) {
      mImageView!!.setImageBitmap(textBmp)
    }
    setTextWidth(textWidth)
    setTextHeight(textHeight)
    return textBmp
  }

  private fun generateFileFromBitmap(bmp: Bitmap, outputPath: String, srcMimeType: String): Boolean {
    var type = srcMimeType
    val outputFile = File(outputPath)
    if (!outputFile.exists()) {
      val dir = outputFile.parentFile ?: return false
      if (!dir.exists() && dir.isDirectory) {
        dir.mkdirs()
      }
      outputFile.createNewFile()
    }
    val outputStream = FileOutputStream(outputFile)
    type = if (TextUtils.isEmpty(srcMimeType)) "jpeg" else srcMimeType
    if (outputPath.endsWith("jpg") || outputPath.endsWith("jpeg")
        || type.endsWith("jpg") || type.endsWith("jpeg")) {
      bmp.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)
    } else if (outputPath.endsWith("png") || type.endsWith("png")) {
      bmp.compress(Bitmap.CompressFormat.PNG, 100, outputStream)
    } else if (outputPath.endsWith("webp") || type.endsWith("webp")) {
      bmp.compress(Bitmap.CompressFormat.WEBP, 100, outputStream)
    } else {
      Log.e("trinity", "not supported image format for '$outputPath'")
      outputStream.flush()
      outputStream.close()
      if (outputFile.exists()) {
        outputFile.delete()
      }
      return false
    }
    outputStream.flush()
    outputStream.close()
    return true
  }

  private fun getLayoutMaxWidth(): Int {
    var maxWidth = -1
    for (i in 0 until mLayout!!.lineCount) {
      if (maxWidth < mLayout!!.getLineWidth(i)) {
        maxWidth = mLayout!!.getLineWidth(i).toInt()
      }
    }
    return maxWidth
  }

  private fun binarySearch(start: Int, end: Int, text: String, sizeTester: SizeTester,
                           availableSpace: RectF): Int {
    var lastBest = start
    var lo = start
    var hi = end - 1
    var mid = 0
    Log.d("trinity", "start : " + start +
        " end : " + end +
        " width : " + availableSpace.right +
        " height : " + availableSpace.bottom)
    while (lo <= hi) {
      mid = lo + hi ushr 1
      val midValCmp = sizeTester.onTestSize(mid, availableSpace, text)
      when {
        midValCmp < 0 -> {
          lastBest = lo
          lo = mid + 1
        }
        midValCmp > 0 -> {
          hi = mid - 1
          lastBest = hi
        }
        else -> {
          return mid
        }
      }
    }
    Log.d("BINARYTEXT", "last best : $lastBest")
    // make sure to return last best
    // this is what should always be returned
    return lastBest
  }

  fun getMaxTextSize(): Float {
    return mMaxTextSize
  }

  fun setEditCompleted(isEditCompleted: Boolean) {
    this.isEditCompleted = isEditCompleted
  }

  fun setTextWidth(mWidth: Int) {
    this.mWidth = mWidth
  }

  fun setTextHeight(mHeight: Int) {
    this.mHeight = mHeight
  }

  fun setTextOnly(isTextOnly: Boolean) {
    this.isTextOnly = isTextOnly
  }

  fun setTextTop(mTop: Int) {
    this.mTop = mTop
  }

  fun setTextBottom(mBottom: Int) {
    this.mBottom = mBottom
  }

  fun setTextLeft(mLeft: Int) {
    this.mLeft = mLeft
  }

  fun setTextRight(mRight: Int) {
    this.mRight = mRight
  }

  fun setFontPath(fontPath: String) {
    mFontPath = fontPath
    if (TextUtils.isEmpty(fontPath)) {
      typeface = Typeface.DEFAULT
    } else {
      if (File(mFontPath).exists()) {
        typeface = Typeface.createFromFile(fontPath)
      }
    }
  }

  fun getFontPath(): String? {
    return mFontPath
  }

  fun restore(width: Int, height: Int) {
    mLastHeight = height
    mLastWidth = width
  }

  fun setCurrentColor(mCurrentColor: Int) {
    this.mCurrentColor = mCurrentColor
    setTextColor(mCurrentColor)
  }

  fun setMirror(isMirror: Boolean) {
    this.isMirror = isMirror
    val angle = Math.toDegrees(mTextAngle.toDouble()).toFloat()
    rotation = if (isMirror) -angle else angle
    requestLayout()
  }

  fun setTextAngle(mTextAngle: Float) {
    this.mTextAngle = mTextAngle
    rotation = Math.toDegrees(mTextAngle.toDouble()).toFloat()
  }

  fun getTextRotation(): Float {
    return mTextAngle
  }

  fun setTextStrokeColor(currentStrokeColor: Int) {
    mCurrentStrokeColor = currentStrokeColor
    invalidate()
  }

  fun getTextColor(): Int {
    return mCurrentColor
  }

  fun getTextStrokeColor(): Int {
    return mCurrentStrokeColor!!
  }

  override fun setText(text: CharSequence, type: BufferType?) {
    super.setText(filtrateText(text), type)
  }

  private fun calculateMaxLinesByText(text: CharSequence): Int {
    if (TextUtils.isEmpty(text)) {
      return 1
    }
    val sb = StringBuilder(text)
    var line = sb.toString().split("\n").toTypedArray().size
    while (sb.isNotEmpty() && sb[sb.length - 1] == '\n') {
      line++
      sb.deleteCharAt(sb.length - 1)
    }
    return line
  }

  private fun filtrateText(text: CharSequence): CharSequence? {
    if (TextUtils.isEmpty(text)) {
      return text
    }
    if (!isTextOnly) {
      val result = count(text.toString())
      if (result[0] > 10) {
        return text.subSequence(0, result[1])
      }
    }
    return text
  }

  private fun isChinese(name: String): Boolean {
    val ch = name.toCharArray();
    ch.forEach {
      val ub = Character.UnicodeBlock.of(it)
      return ub === Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS
          || ub === Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS
          || ub === Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
          || ub === Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B
          || ub === Character.UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION
          || ub === Character.UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS
          || ub === Character.UnicodeBlock.GENERAL_PUNCTUATION
    }
    return false
  }

  private fun count(text: String): IntArray {
    val len = text.length
    var skip: Int
    var letter = 0
    var chinese = 0
    var count = 0
    var limit = 0
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
      count = (if (letter % 2 == 0) letter / 2 else letter / 2 + 1) + chinese
      if (count == 10) {
        limit = i + 1
      }
      i += skip
    }
    val result = IntArray(2)
    result[0] = count
    result[1] = limit
    return result
  }

  fun getmStrokeWidth(): Float {
    return mStrokeWidth
  }

  override fun getMaxLines(): Int {
    return mMaxLines
  }

  override fun setMaxLines(maxlines: Int) {
    super.setMaxLines(maxlines)
    mMaxLines = maxlines
    reAdjust()
  }

  override fun setSingleLine() {
    super.setSingleLine()
    mMaxLines = 1
    reAdjust()
  }

  override fun setSingleLine(singleLine: Boolean) {
    super.setSingleLine(singleLine)
    if (singleLine) {
      mMaxLines = 1
    } else {
      mMaxLines = NO_LINE_LIMIT
    }
    reAdjust()
  }

  override fun setLines(lines: Int) {
    super.setLines(lines)
    mMaxLines = lines
    reAdjust()
  }

  override fun setLineSpacing(add: Float, mult: Float) {
    super.setLineSpacing(add, mult)
    mSpacingMult = mult
    mSpacingAdd = add
  }

  /**
   * Set the lower text size limit and invalidate the view
   *
   * @param minTextSize
   */
  fun setMinTextSize(minTextSize: Float) {
    mMinTextSize = minTextSize
    reAdjust()
  }

  fun reAdjust() {
    adjustTextSize()
  }

  private fun adjustTextSize() {
    if (!mInitialized) {
      return
    }
    if (TextUtils.isEmpty(text)) {
      return
    }
    val startSize = mMinTextSize.toInt()
    var maxTextSize = 0f
    maxTextSize = if (isEditCompleted) {
      mMaxTextSize
    } else {
      val base = textSize
      val minLimit = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 10f, resources.displayMetrics)
      max(base, minLimit)
    }
    mWidthLimit = mAvailableSpaceRect!!.right.toInt()
    //处理偶现计算行数不准确的问题
    mWidthLimit = if (mWidthLimit < 1) mWidthLimit else mWidthLimit - 1
    Log.i("mWidthLimit", "mWidthLimit : $mWidthLimit")
    mPaint = TextPaint(paint)
    super.setTextSize(
        TypedValue.COMPLEX_UNIT_PX,
        efficientTextSizeSearch(startSize, maxTextSize.toInt(),
            mSizeTester, mAvailableSpaceRect!!).toFloat())
  }

  private fun efficientTextSizeSearch(start: Int, end: Int,
                                      sizeTester: SizeTester, availableSpace: RectF): Int {
    var text = text.toString()
    if (isTextOnly) {
      //添加10个字换行
      text = TextDialog.lineFeedText(getText().toString()) ?: ""
    }
    return binarySearch(start, end, text, sizeTester, availableSpace)
  }

  protected fun isNeedSelfMeasure(): Boolean {
    return !isEditCompleted
  }

  override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
    val w: Int
    val h: Int
    if (isNeedSelfMeasure()) {
      super.onMeasure(ViewGroup.LayoutParams.WRAP_CONTENT,
          ViewGroup.LayoutParams.WRAP_CONTENT)
      w = measuredWidth
      h = measuredHeight
      setMeasuredDimension(w + 30, h + 30)
      if (w != mMaxWidthWhenOutof || h != mMaxHeightWhenOutof) {
        return
      }
    } else {
      w = MeasureSpec.getSize(widthMeasureSpec)
      h = MeasureSpec.getSize(heightMeasureSpec)
    }
    if (mLastHeight == 0 || mLastWidth == 0) {
      mLastHeight = h
      mLastWidth = w
    }
    setMeasuredDimension(w, h)
    measureSelf(w, h)
  }

  protected fun measureSelf(width: Int, height: Int) {
    if (width == 0 || height == 0) {
      return
    }
    var w: Int
    var h: Int
    var top = 0
    var left = 0
    var right = 0
    var bottom = 0
    val rotx: Float
    val roty: Float
    if (mLastHeight == 0 || mLastWidth == 0) {
      rotx = 1f
      roty = 1f
    } else {
      rotx = width.toFloat() / mLastWidth.toFloat()
      roty = height.toFloat() / mLastHeight.toFloat()
    }
    if (mWidth == 0 || mHeight == 0) {
      mWidth = mLastWidth
      mHeight = mLastHeight
    }
    w = (mWidth * rotx).toInt()
    h = (mHeight * roty).toInt()
    left = (mLeft * rotx).toInt()
    top = (mTop * roty).toInt()
    right = (mRight * rotx).toInt()
    bottom = (mBottom * roty).toInt()
    if (isMirror) {
      val temp = left
      left = right
      right = temp
    }
    if (w == 0 || h == 0) {
      w = width
      h = height
    }
    mAvailableSpaceRect!!.right = w.toFloat()
    mAvailableSpaceRect!!.bottom = h.toFloat()
    adjustTextSize()
    setPadding(left, top, right, bottom)
  }

  override fun onDraw(canvas: Canvas?) {
    if (mScaleByDrawable) {
      val alpha = paint.alpha
      paint.alpha = 0
      super.onDraw(canvas)
      paint.alpha = alpha
      return
    }
    freeze()
    val restoreColor = this.currentTextColor
    setShadowLayer(0f, 0f, 0f, 0)
    if (mCurrentStrokeColor != null && mCurrentStrokeColor != 0) {
      val paint = this.paint
      paint.style = Paint.Style.STROKE
      paint.strokeJoin = mStrokeJoin
      paint.strokeMiter = mStrokeMiter
      this.setTextColor(mCurrentStrokeColor ?: Color.WHITE)
      mStrokeWidth = generateStrokeWidth()
      paint.strokeWidth = mStrokeWidth
      super.onDraw(canvas)
      paint.style = Paint.Style.FILL
    }
    this.setTextColor(restoreColor)
    unfreeze()
    super.onDraw(canvas)
  }

  private fun generateStrokeWidth(): Float {
    val value = textSize
    var size = px2sp(context, value)
    val strokeWidth = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1f, resources.displayMetrics)
    return if (size <= 27) {
      strokeWidth
    } else {
      size -= 27
      var width = strokeWidth * (size.toFloat() / 15f + 1)
      if (VERSION.SDK_INT == VERSION_CODES.KITKAT) {
        if (value > 256) {
          width /= 5f
        }
      }
      width
    }
  }

  /**
   * 将px值转换为sp值，保证文字大小不变
   */
  fun px2sp(context: Context, pxValue: Float): Int {
    val fontScale = context.resources.displayMetrics.scaledDensity
    return (pxValue / fontScale + 0.5f).toInt()
  }

  // Keep these things locked while onDraw in processing
  fun freeze() {
    mFrozen = true
  }

  fun unfreeze() {
    mFrozen = false
  }


  override fun requestLayout() {
    if (!mFrozen) {
      super.requestLayout()
    }
  }

  override fun postInvalidate() {
    if (!mFrozen) {
      super.postInvalidate()
    }
  }

  override fun postInvalidate(left: Int, top: Int, right: Int, bottom: Int) {
    if (!mFrozen) {
      super.postInvalidate(left, top, right, bottom)
    }
  }

  override fun invalidate() {
    if (!mFrozen) {
      super.invalidate()
    }
  }

  override fun invalidate(rect: Rect?) {
    if (!mFrozen) {
      super.invalidate(rect)
    }
  }

  override fun invalidate(l: Int, t: Int, r: Int, b: Int) {
    if (!mFrozen) {
      super.invalidate(l, t, r, b)
    }
  }

  /**
   * 该方法会在非UI线程被调用，因此不能执行UI更新操作
   * @param bmpWidth
   * @param bmpHeight
   * @return
   */
  fun generateBitmap(bmpWidth: Int, bmpHeight: Int): Bitmap? {
    if (mBitmapGenerator == null) {
      mTextBitmap = TextBitmap()
      mBitmapGenerator = TextBitmapGenerator()
    }
    mTextBitmap?.mText = text.toString()
    mTextBitmap?.mFontPath = mFontPath!!
    mTextBitmap?.mBmpWidth = bmpWidth
    mTextBitmap?.mBmpHeight = bmpHeight
    mTextBitmap?.mTextWidth = bmpWidth
    mTextBitmap?.mTextHeight = bmpHeight
    mTextBitmap?.mTextColor = mCurrentColor
    mTextBitmap?.mTextStrokeColor = mCurrentStrokeColor!!
    mTextBitmap?.mTextAlignment = Layout.Alignment.ALIGN_CENTER
    mBitmapGenerator?.updateTextBitmap(mTextBitmap)
    return mBitmapGenerator?.generateBitmap(bmpWidth, bmpHeight)
  }

  fun getTextWidth(): Int {
    var layout = layout
    if (layout == null) {
      measureSelf(mWidth, mHeight)
      val w = mWidth + paddingLeft + paddingRight
      val h = mHeight + paddingTop + paddingBottom
      super.measure(MeasureSpec.makeMeasureSpec(w, MeasureSpec.EXACTLY),
          MeasureSpec.makeMeasureSpec(h, MeasureSpec.EXACTLY))
      layout = getLayout()
    }
    if (layout == null) {
      layout = mLayout
      if (layout == null) {
        return 0
      }
    }
    return layout.width
  }

  fun getTextHeight(): Int {
    var layout = layout
    if (layout == null) {
      measureSelf(mWidth, mHeight)
      val w = mWidth + paddingLeft + paddingRight
      val h = mHeight + paddingTop + paddingBottom
      super.measure(MeasureSpec.makeMeasureSpec(w, MeasureSpec.EXACTLY),
          MeasureSpec.makeMeasureSpec(h, MeasureSpec.EXACTLY))
      layout = getLayout()
    }
    if (layout == null) {
      layout = mLayout
      if (layout == null) {
        return 0
      }
    }
    return layout.height
  }

  private interface SizeTester {
    /**
     * @param suggestedSize  Size of text to be tested
     * @param availableSpace available space in which text must fit
     * @return an integer < 0 if after applying `suggestedSize` to
     * text, it takes less space than `availableSpace`, > 0
     * otherwise
     */
    fun onTestSize(suggestedSize: Int, availableSpace: RectF?, text: String?): Int
  }
}