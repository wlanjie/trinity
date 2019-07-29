package com.trinity.sample.view;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import com.trinity.sample.R;

public class TabItem extends View {
  public final CharSequence text;
  public final Drawable icon;
  public final int customLayout;

  public TabItem(Context context) {
    this(context, null);
  }

  public TabItem(Context context, AttributeSet attrs) {
    super(context, attrs);

    final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TrinityTabItem, 0, 0);
    text = a.getText(R.styleable.TrinityTabItem_android_text);
    icon = a.getDrawable(R.styleable.TrinityTabItem_android_icon);
    customLayout = a.getResourceId(R.styleable.TrinityTabItem_android_layout, 0);
    a.recycle();
  }
}
