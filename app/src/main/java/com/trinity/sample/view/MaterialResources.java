package com.trinity.sample.view;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.TypedValue;
import androidx.annotation.Nullable;
import androidx.annotation.StyleableRes;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.appcompat.widget.TintTypedArray;
import com.google.android.material.resources.TextAppearance;

public class MaterialResources {

  private MaterialResources() {}

  /**
   * Returns the {@link ColorStateList} from the given {@link TypedArray} attributes. The resource
   * can include themeable attributes, regardless of API level.
   */
  @Nullable
  public static ColorStateList getColorStateList(
      Context context, TypedArray attributes, @StyleableRes int index) {
    if (attributes.hasValue(index)) {
      int resourceId = attributes.getResourceId(index, 0);
      if (resourceId != 0) {
        ColorStateList value = AppCompatResources.getColorStateList(context, resourceId);
        if (value != null) {
          return value;
        }
      }
    }

    // Reading a single color with getColorStateList() on API 15 and below doesn't always correctly
    // read the value. Instead we'll first try to read the color directly here.
    if (VERSION.SDK_INT <= VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
      int color = attributes.getColor(index, -1);
      if (color != -1) {
        return ColorStateList.valueOf(color);
      }
    }

    return attributes.getColorStateList(index);
  }

  /**
   * Returns the {@link ColorStateList} from the given {@link TintTypedArray} attributes. The
   * resource can include themeable attributes, regardless of API level.
   */
  @Nullable
  public static ColorStateList getColorStateList(
      Context context, TintTypedArray attributes, @StyleableRes int index) {
    if (attributes.hasValue(index)) {
      int resourceId = attributes.getResourceId(index, 0);
      if (resourceId != 0) {
        ColorStateList value = AppCompatResources.getColorStateList(context, resourceId);
        if (value != null) {
          return value;
        }
      }
    }

    // Reading a single color with getColorStateList() on API 15 and below doesn't always correctly
    // read the value. Instead we'll first try to read the color directly here.
    if (VERSION.SDK_INT <= VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
      int color = attributes.getColor(index, -1);
      if (color != -1) {
        return ColorStateList.valueOf(color);
      }
    }

    return attributes.getColorStateList(index);
  }

  /**
   * Returns the drawable object from the given attributes.
   *
   * <p>This method supports inflation of {@code <vector>} and {@code <animated-vector>} resources
   * on devices where platform support is not available.
   */
  @Nullable
  public static Drawable getDrawable(
      Context context, TypedArray attributes, @StyleableRes int index) {
    if (attributes.hasValue(index)) {
      int resourceId = attributes.getResourceId(index, 0);
      if (resourceId != 0) {
        Drawable value = AppCompatResources.getDrawable(context, resourceId);
        if (value != null) {
          return value;
        }
      }
    }
    return attributes.getDrawable(index);
  }

  /**
   * Returns a TextAppearanceSpan object from the given attributes.
   *
   * <p>You only need this if you are drawing text manually. Normally, TextView takes care of this.
   */
  @Nullable
  public static TextAppearance getTextAppearance(
      Context context, TypedArray attributes, @StyleableRes int index) {
    if (attributes.hasValue(index)) {
      int resourceId = attributes.getResourceId(index, 0);
      if (resourceId != 0) {
        return new TextAppearance(context, resourceId);
      }
    }
    return null;
  }

  /**
   * Retrieve a dimensional unit attribute at <var>index</var> for use as a size in raw pixels. A
   * size conversion involves rounding the base value, and ensuring that a non-zero base value is at
   * least one pixel in size.
   *
   * <p>This method will throw an exception if the attribute is defined but is not a dimension.
   *
   * @param context The Context the view is running in, through which the current theme, resources,
   *     etc can be accessed.
   * @param attributes array of typed attributes from which the dimension unit must be read.
   * @param index Index of attribute to retrieve.
   * @param defaultValue Value to return if the attribute is not defined or not a resource.
   * @return Attribute dimension value multiplied by the appropriate metric and truncated to integer
   *     pixels, or defaultValue if not defined.
   * @throws UnsupportedOperationException if the attribute is defined but is not a dimension.
   * @see TypedArray#getDimensionPixelSize(int, int)
   */
  public static int getDimensionPixelSize(
      Context context, TypedArray attributes, @StyleableRes int index, final int defaultValue) {
    TypedValue value = new TypedValue();
    if (!attributes.getValue(index, value) || value.type != TypedValue.TYPE_ATTRIBUTE) {
      return attributes.getDimensionPixelSize(index, defaultValue);
    }

    TypedArray styledAttrs = context.getTheme().obtainStyledAttributes(new int[] {value.data});
    int dimension = styledAttrs.getDimensionPixelSize(0, defaultValue);
    styledAttrs.recycle();
    return dimension;
  }

  /**
   * Returns the @StyleableRes index that contains value in the attributes array. If both indices
   * contain values, the first given index takes precedence and is returned.
   */
  @StyleableRes
  static int getIndexWithValue(TypedArray attributes, @StyleableRes int a, @StyleableRes int b) {
    if (attributes.hasValue(a)) {
      return a;
    }
    return b;
  }
}
