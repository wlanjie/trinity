package com.trinity.sample

import android.app.Application
import com.tencent.bugly.crashreport.CrashReport

/**
 * Created by wlanjie on 2019-07-19
 */
class TrinityApplication : Application() {

  companion object {
    init {
      System.loadLibrary("trinity")
    }
  }

  override fun onCreate() {
    super.onCreate()

    CrashReport.initCrashReport(applicationContext, "c998b23a2d", true)
  }
}