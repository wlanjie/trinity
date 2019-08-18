package com.trinity.listener

/**
 * Created by wlanjie on 2019-07-31
 */
interface OnExportListener {

  fun onExportProgress(progress: Float)
  fun onExportFailed(error: Int)
  fun onExportCanceled()
  fun onExportComplete()
}