package com.trinity.sample.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View
import androidx.annotation.Nullable
import com.trinity.sample.R
import java.util.*

class LineProgressView : View {

    private var mPaint: Paint? = null

    private var mRadius = RADIUS.toFloat()

    private var mBackgroundColor = BACKGROUND_COLOR

    private var mContentColor = CONTENT_COLOR

    private var mDividerColor = DIVIDER_COLOR

    private var mDividerWidth = DIVIDER_WIDTH

    private var mLoadingProgress: Float = 0.toFloat()

    private val mProgressList = ArrayList<Float>()

    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, @Nullable attrs: AttributeSet) : super(context, attrs) {
        init()
        val ta = context.obtainStyledAttributes(attrs, R.styleable.ProgressView)
        try {
            mRadius = ta.getDimensionPixelSize(R.styleable.ProgressView_pv_radius, RADIUS).toFloat()
            mBackgroundColor = ta.getColor(R.styleable.ProgressView_pv_bg_color, BACKGROUND_COLOR)
            mContentColor = ta.getColor(R.styleable.ProgressView_pv_content_color, CONTENT_COLOR)
            mDividerColor = ta.getColor(R.styleable.ProgressView_pv_divider_color, DIVIDER_COLOR)
            mDividerWidth =
                ta.getDimensionPixelSize(R.styleable.ProgressView_pv_divider_width, DIVIDER_WIDTH)
        } finally {
            ta.recycle()
        }
    }

    private fun init() {
        mPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    }

    override fun onDraw(canvas: Canvas) {
        drawBackground(canvas)
        drawContent(canvas)
        drawDivider(canvas)
    }

    private fun drawBackground(canvas: Canvas) {
        mPaint!!.color = mBackgroundColor
        canvas.drawRoundRect(
            RectF(0f, 0f, measuredWidth.toFloat(), measuredHeight.toFloat()),
            mRadius, mRadius, mPaint!!
        )
    }

    private fun drawContent(canvas: Canvas) {
        var total = 0f
        for (progress in mProgressList) {
            total += progress
        }
        total += mLoadingProgress
        val width = (total * measuredWidth).toInt()
        mPaint!!.color = mContentColor
        canvas.drawRoundRect(
            RectF(0f, 0f, width.toFloat(), measuredHeight.toFloat()),
            mRadius, mRadius, mPaint!!
        )
        if (width < mRadius) {
            return
        }
        canvas.drawRect(RectF(mRadius, 0f, width.toFloat(), measuredHeight.toFloat()), mPaint!!)
    }

    private fun drawDivider(canvas: Canvas) {
        mPaint!!.color = mDividerColor
        var left = 0
        for (progress in mProgressList) {
            left += (progress * measuredWidth).toInt()
            canvas.drawRect(
                (left - mDividerWidth).toFloat(),
                0f,
                left.toFloat(),
                measuredHeight.toFloat(),
                mPaint!!
            )
        }
    }

    fun setLoadingProgress(loadingProgress: Float) {
        mLoadingProgress = loadingProgress
        invalidate()
    }

    fun addProgress(progress: Float) {
        mLoadingProgress = 0f
        mProgressList.add(progress)
        invalidate()
    }

    fun deleteProgress() {
        if (mProgressList.isEmpty()) {
            return
        }
        mProgressList.removeAt(mProgressList.size - 1)
        invalidate()
    }

    fun clear() {
        mProgressList.clear()
        invalidate()
    }

    companion object {

        private const val RADIUS = 4

        private const val DIVIDER_WIDTH = 2

        private val BACKGROUND_COLOR = Color.parseColor("#22000000")

        private val CONTENT_COLOR = Color.parseColor("#face15")

        private const val DIVIDER_COLOR = Color.WHITE
    }
}