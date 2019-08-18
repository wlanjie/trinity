package com.trinity.sample.fragment

import android.os.Build
import android.os.Bundle
import android.provider.MediaStore
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.trinity.sample.R
import com.trinity.sample.adapter.MediaAdapter
import com.trinity.sample.entity.MediaItem
import com.trinity.sample.view.SpacesItemDecoration
import kotlinx.android.synthetic.main.item_media.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.*

class PictureFragment : Fragment() {

  private val mAdapter = MediaAdapter()

  companion object {
    private const val DURATION = "duration"
    private val PROJECTION = arrayOf(MediaStore.Files.FileColumns._ID, MediaStore.MediaColumns.DATA, MediaStore.MediaColumns.MIME_TYPE, MediaStore.MediaColumns.WIDTH, MediaStore.MediaColumns.HEIGHT, DURATION)
    private const val ORDER_BY = MediaStore.Files.FileColumns._ID + " DESC"
    private val QUERY_URI = MediaStore.Files.getContentUri("external")
    private const val SELECTION_NOT_GIF = (MediaStore.Files.FileColumns.MEDIA_TYPE + "=?"
        + " AND " + MediaStore.MediaColumns.SIZE + ">0"
        + " AND " + MediaStore.MediaColumns.MIME_TYPE + "!='image/gif'")
  }

  override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
    return inflater.inflate(R.layout.fragment_video, container, false)
  }

  override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
    super.onViewCreated(view, savedInstanceState)
    val recyclerView = view.findViewById<RecyclerView>(R.id.recycler_view)
    recyclerView.layoutManager = GridLayoutManager(activity, 4)
    recyclerView.adapter = mAdapter
    recyclerView.addItemDecoration(SpacesItemDecoration(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 2f, resources.displayMetrics).toInt(), 4))
    GlobalScope.launch(Dispatchers.IO) {
      loadPicture()
    }
  }

  fun getSelectMedias(): MutableList<MediaItem> {
    return mAdapter.getSelectMedia()
  }

  private suspend fun loadPicture() {
    val medias = mutableListOf<MediaItem>()
    activity?.let {
      val cursor = it.contentResolver.query(QUERY_URI,
          PROJECTION,
          SELECTION_NOT_GIF,
          arrayOf(MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString()), ORDER_BY)
      while (cursor?.moveToNext() == true) {
        val id = cursor.getLong(cursor.getColumnIndexOrThrow(PROJECTION[0]))
        val path = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
          QUERY_URI.buildUpon().appendPath(id.toString()).build().toString()
        } else {
          cursor.getString(cursor.getColumnIndexOrThrow(PROJECTION[1]))
        }
        val pictureType = cursor.getString(cursor.getColumnIndexOrThrow(PROJECTION[2]))
        val width = cursor.getInt(cursor.getColumnIndexOrThrow(PROJECTION[3]))
        val height = cursor.getInt(cursor.getColumnIndexOrThrow(PROJECTION[4]))
        val duration = cursor.getInt(cursor.getColumnIndexOrThrow(PROJECTION[5]))
        val item = MediaItem(path, pictureType, width, height, duration)
        medias.add(item)
      }
      cursor?.close()

      withContext(Dispatchers.Main) {
        mAdapter.addMedias(medias)
      }
    }
  }
}