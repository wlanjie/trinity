package com.trinity.sample.view.foucs

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.content.Context
import android.graphics.PointF
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.trinity.sample.R

class DefaultAutoFocusMarker : AutoFocusMarker {

    private var mContainer: View? = null
    private var mFill: View? = null

    override fun onAttach(context: Context, container: ViewGroup): View? {
        val view = LayoutInflater.from(context).inflate(R.layout.cameraview_layout_focus_marker, container, false)
        mContainer = view.findViewById(R.id.focusMarkerContainer)
        mFill = view.findViewById(R.id.focusMarkerFill)
        return view
    }

    override fun onAutoFocusStart(trigger: AutoFocusTrigger, point: PointF) {
        if (trigger === AutoFocusTrigger.METHOD) return
        mContainer?.clearAnimation()
        mFill?.clearAnimation()
        mContainer?.scaleX = 1.36f
        mContainer?.scaleY = 1.36f
        mContainer?.alpha = 1f
        mFill?.scaleX = 0f
        mFill?.scaleY = 0f
        mFill?.alpha = 1f
        animate(mContainer, 1f, 1f, 300, 0, null)
        animate(mFill, 1f, 1f, 300, 0, null)
    }

    override fun onAutoFocusEnd(trigger: AutoFocusTrigger, successful: Boolean, point: PointF) {
        if (trigger === AutoFocusTrigger.METHOD) return
        if (successful) {
            animate(mContainer, 1f, 0f, 500, 0, null)
            animate(mFill, 1f, 0f, 500, 0, null)
        } else {
            animate(mFill, 0f, 0f, 500, 0, null)
            animate(mContainer, 1.36f, 1f, 500, 0, object : AnimatorListenerAdapter() {
                override fun onAnimationEnd(animation: Animator) {
                    super.onAnimationEnd(animation)
                    animate(mContainer, 1.36f, 0f, 200, 1000, null)
                }
            })
        }
    }

    private fun animate(
        view: View?, scale: Float, alpha: Float, duration: Long,
        delay: Long, listener: Animator.AnimatorListener?
    ) {
        view?.animate()?.scaleX(scale)?.scaleY(scale)?.alpha(alpha)?.setDuration(duration)?.setStartDelay(delay)?.setListener(listener)?.start()
    }
}
