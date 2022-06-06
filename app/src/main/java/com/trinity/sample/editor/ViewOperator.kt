package com.trinity.sample.editor

import android.animation.ObjectAnimator
import android.animation.ValueAnimator
import android.util.TypedValue
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.TranslateAnimation
import android.widget.RelativeLayout
import kotlin.math.abs

/**
 * Created by wlanjie on 2019-07-24
 */
class ViewOperator(
    private val rootView: RelativeLayout,
    private val titleView: ViewGroup,
    private val playerView: View,
    private val bottomMenuView: View,
    private val pasterContainerView: View,
    private val playerButton: View
) {

  private var mBottomView: Chooser ?= null
  private var mPlayerButtonMarginBottom = 0
  private var mPlayerViewMarginTop = 0
  private var mRootViewHeight = 0
  private var mPlayerWidth = 0
  private var mPlayerHeight = 0
  private var mBottomViewHeight = 0
  private var mConfirmViewHeight = 0
  private var mMoveLength = 0
  private var mTranslationY = 0
  private var mTranslationYMax = -1000
  private var mScaleSize = 0.6f
  private var mAnimatorListener: AnimatorListener ?= null

  fun showBottomView(bottomView: Chooser) {
    if (mBottomView != null) {
      return
    }

    bottomMenuView.visibility = View.GONE
    val params: ViewGroup.LayoutParams = playerView.layoutParams
    mPlayerViewMarginTop = if (params is ViewGroup.MarginLayoutParams) {
      params.topMargin
    } else {
      0
    }

    mRootViewHeight = rootView.height
    mPlayerWidth = playerView.width
    mPlayerHeight = playerView.height
    mBottomViewHeight = bottomView.getCalculateHeight()
    mConfirmViewHeight = titleView.height
    mMoveLength = abs(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 10.0f, rootView.resources.displayMetrics) - mPlayerViewMarginTop).toInt()
    mTranslationY = (mPlayerButtonMarginBottom - TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 10.0f, rootView.resources.displayMetrics) - mBottomViewHeight).toInt()
    if (mTranslationY in (mTranslationYMax + 1)..-1) {
      val animator = ObjectAnimator.ofFloat(playerButton, "translationY", 0.0f, mTranslationY.toFloat())
      animator.duration = 250
      animator.start()
    }
    if (mBottomView == bottomView) {
      return
    }
    startAnimOnTop(titleView)
    for (i in 0 until titleView.childCount) {
      titleView.getChildAt(i).isClickable = false
    }

    if (bottomView.isPlayerNeedZoom()) {
      val h = mRootViewHeight - mBottomViewHeight - TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20f, rootView.resources.displayMetrics)
      mScaleSize = h / mPlayerHeight
      if (mScaleSize >= 0.95f) {
        mScaleSize = 0.95f
      }
      val anim = ValueAnimator.ofFloat(1f, mScaleSize)
      anim.duration = 250
      anim.addUpdateListener {
        val currentValue = it.animatedValue as Float
        val layoutParams = playerView.layoutParams
        layoutParams.height = (mPlayerHeight * currentValue).toInt()
        layoutParams.width = (mPlayerWidth * currentValue).toInt()

        val marginParams = if (layoutParams is ViewGroup.MarginLayoutParams) {
          layoutParams
        } else {
          ViewGroup.MarginLayoutParams(layoutParams)
        }
        val marginTop = abs(mPlayerViewMarginTop - mMoveLength * (1 - currentValue) / (1 - mScaleSize))
        marginParams.setMargins(0, marginTop.toInt(), 0, 0)
        playerView.layoutParams = marginParams
        pasterContainerView.layoutParams = marginParams
        if (currentValue == mScaleSize && mAnimatorListener != null) {
          mAnimatorListener?.onShowAnimationEnd()
        }
      }
      anim.start()
    }
    val layoutParams = if (bottomView is MusicChooser) {
      RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
    } else {
      RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
    }
    layoutParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM)
    rootView.addView(bottomView, layoutParams)
    startAnimY(bottomView)
    mBottomView = bottomView
  }

  fun hideBottomView() {
    if (mBottomView == null) {
      return
    }
    startAnimY(bottomMenuView)
    bottomMenuView.visibility = View.VISIBLE
    startAnimOnTop(titleView)
    titleView.visibility = View.VISIBLE
    for (i in 0 until titleView.childCount) {
      titleView.getChildAt(i).isClickable = true
    }
    if (mTranslationYMax < mTranslationY && mTranslationY < 0) {
      val animator = ObjectAnimator.ofFloat(playerButton, "translationY", mTranslationY.toFloat(), 0f)
      animator.duration = 250
      animator.start()
    }
    if (mBottomView?.isPlayerNeedZoom() == true) {
      val animator = ValueAnimator.ofFloat(mScaleSize, 1f)
      animator.duration = 250
      animator.addUpdateListener {
        val currentValue = it.animatedValue as Float
        val params = playerView.layoutParams
        params.height = (mPlayerHeight * currentValue).toInt()
        params.width = (mPlayerWidth * currentValue).toInt()
        val marginParams = if (params is ViewGroup.MarginLayoutParams) {
          params
        } else {
          ViewGroup.MarginLayoutParams(params)
        }
        val marginTop = abs(mPlayerViewMarginTop - mMoveLength) * (1 - currentValue) / (1 - mScaleSize)
        marginParams.setMargins(0, marginTop.toInt(), 0, 0)
        playerView.layoutParams = marginParams
        pasterContainerView.layoutParams = marginParams
        if (currentValue == 1f && mAnimatorListener != null) {
          mAnimatorListener?.onHideAnimationEnd()
        }
      }
      animator.start()
    }
    mBottomView?.apply {
      startDisappearAnimY(this)
      removeOwn()
      mBottomView = null
    }
  }

  fun hideBottomEditorView(page: EditorPage) {
    when (page) {
      EditorPage.FILTER, EditorPage.SOUND, EditorPage.MV -> {
        hideBottomView()
      }
    }
  }

  fun isBottomViewShow(): Boolean = mBottomView != null

  fun getBottomView(): Chooser? {
    return mBottomView
  }

  private fun startDisappearAnimY(view: View) {
    val hideAnim = TranslateAnimation(Animation.RELATIVE_TO_SELF, 0f,
      Animation.RELATIVE_TO_SELF, 0f,
      Animation.RELATIVE_TO_SELF, 0f,
      Animation.RELATIVE_TO_SELF, 1f)
    hideAnim.duration = 250
    view.startAnimation(hideAnim)
  }

  private fun startAnimY(view: View) {
    val showAnim = TranslateAnimation(Animation.RELATIVE_TO_SELF, 0f,
        Animation.RELATIVE_TO_SELF, 0f,
        Animation.RELATIVE_TO_SELF, 1f,
        Animation.RELATIVE_TO_SELF, 0f)
    showAnim.duration = 250
    view.startAnimation(showAnim)
  }

  private fun startAnimOnTop(view: View) {
    val hideAnim = TranslateAnimation(Animation.RELATIVE_TO_SELF, 0f,
        Animation.RELATIVE_TO_SELF, 0f,
        Animation.RELATIVE_TO_SELF, 0f,
        Animation.RELATIVE_TO_SELF, -1f)
    hideAnim.duration = 250
    hideAnim.setAnimationListener(object: Animation.AnimationListener {
      override fun onAnimationRepeat(animation: Animation?) {
      }

      override fun onAnimationEnd(animation: Animation?) {
        view.visibility = View.GONE
      }

      override fun onAnimationStart(animation: Animation?) {
      }

    })
  }

  fun setAnimatorListener(l: AnimatorListener) {
    mAnimatorListener = l
  }

  interface AnimatorListener {
    fun onShowAnimationEnd()
    fun onHideAnimationEnd()
  }
}