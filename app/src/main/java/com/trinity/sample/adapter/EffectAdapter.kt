package com.trinity.sample.adapter

import android.content.Context
import android.graphics.Color
import android.view.*
import android.widget.ImageView
import androidx.core.view.MotionEventCompat
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R
import com.trinity.sample.entity.Effect
import com.trinity.sample.view.CircleTextView
import de.hdodenhof.circleimageview.CircleImageView

/**
 * Created by wlanjie on 2019-07-26
 */
class EffectAdapter(context: Context, effects: List<Effect>) : RecyclerView.Adapter<EffectAdapter.ViewHolder>(), View.OnTouchListener {

  private var mAdding = false
  private var mPressView: View? = null
  private val mEffects = effects
  private var mTouchListener: OnItemTouchListener ?= null
  private val mDetector = GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {

  override fun onShowPress(e: MotionEvent?) {
      super.onShowPress(e)
      if (mAdding) {
        return
      }
      if (mPressView != null) {
        val holder = mPressView?.tag as ViewHolder
        val position = holder.adapterPosition
        holder.image.visibility = View.GONE
        holder.select.visibility = View.VISIBLE
        holder.image.isSelected = true
        val effect = mEffects[position]
//        mSelectedPos = position
//        mSelectedHolder = holder
//        val effectInfo = EffectInfo()
//        effectInfo.type = UIEditorPage.FILTER_EFFECT
//        effectInfo.setPath(mFilterList.get(position))
//        effectInfo.id = position
        mTouchListener?.onTouchEvent(MotionEvent.ACTION_DOWN, position, effect)
        mAdding = true
      }
    }
  })

  override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
    val view = LayoutInflater.from(parent.context).inflate(R.layout.resource_item_view, parent, false)
    return ViewHolder(view)
  }

  override fun getItemCount(): Int {
    return mEffects.size
  }

  override fun onBindViewHolder(holder: ViewHolder, position: Int) {
    val effect = mEffects[position]
    holder.image.visibility = View.GONE
    holder.effectName.visibility = View.VISIBLE
    holder.image.setImageResource(R.mipmap.ic_launcher_round)
    holder.effectName.text = effect.name
    holder.effectName.setColor(Color.parseColor(effect.color))
    holder.itemView.setOnTouchListener(this)
    holder.itemView.tag = holder
  }

  class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val image: CircleImageView = itemView.findViewById(R.id.resource_image_view)
    val select: ImageView = itemView.findViewById(R.id.select_state)
    val effectName: CircleTextView = itemView.findViewById(R.id.effect_name)
  }

  override fun onTouch(v: View?, event: MotionEvent?): Boolean {
    mDetector.onTouchEvent(event)
    when (event?.action) {
      MotionEvent.ACTION_CANCEL, MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
//        if (!mAdding) {
//          mPressView = null
//          return true
//        }
        if (v != mPressView) {
          return true
        }
        val holder = v?.tag as ViewHolder
        val position = holder.adapterPosition
        val effect = mEffects[position]
        mTouchListener?.onTouchEvent(MotionEvent.ACTION_UP, position, effect)
        holder.image.visibility = View.VISIBLE
        holder.select.visibility = View.GONE
        mAdding = false
        mPressView = null
      }
      MotionEvent.ACTION_DOWN -> {
        if (mAdding) {
          return false
        }
        if (mPressView == null) {
          mPressView = v
        }
      }
    }
    return true
  }

  fun setOnTouchListener(l: OnItemTouchListener?) {
    mTouchListener = l
  }

  interface OnItemTouchListener {
    fun onTouchEvent(event: Int, position: Int, effect: Effect)
  }
}