@file:Suppress("DEPRECATION")

package com.trinity.sample

import android.app.Application
import android.content.Context
import android.os.Environment
import android.text.TextUtils
import com.tencent.bugly.crashreport.CrashReport
import com.tencent.mars.xlog.Log
import com.tencent.mars.xlog.Xlog
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import java.io.*


/**
 * Created by wlanjie on 2019-07-19
 */
class TrinityApplication : Application() {

  companion object {
    init {
      System.loadLibrary("trinity")
      System.loadLibrary("c++_shared")
      System.loadLibrary("marsxlog")
    }
  }

  override fun onCreate() {
    super.onCreate()

//    CrashReport.initCrashReport(applicationContext, "c998b23a2d", true)
    val filterLocalDir = externalCacheDir?.absolutePath + "/filter/"
    val file = File(filterLocalDir)
    if (!file.exists()) {
      GlobalScope.launch(Dispatchers.IO) {
        copyAssets("filter", filterLocalDir)
      }
    }

    val effectLocalDir = externalCacheDir?.absolutePath + "/effect"
    val effectDir = File(effectLocalDir)
    if (!effectDir.exists()) {
      GlobalScope.launch(Dispatchers.IO) {
        copyAssets("effect", effectLocalDir)
      }
    }

    val logPath = Environment.getExternalStorageDirectory().absolutePath + "/trinity"
    if (BuildConfig.DEBUG) {
      Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
      Xlog.setConsoleLogOpen(true)
    } else {
      Xlog.appenderOpen(Xlog.LEVEL_DEBUG, Xlog.AppednerModeAsync, "", logPath, "trinity", 0, "")
      Xlog.setConsoleLogOpen(false)
    }
    Log.setLogImp(Xlog())
  }

  private fun copy(source: String, targetPath: String) {
    if (TextUtils.isEmpty(source) || TextUtils.isEmpty(targetPath)) {
      return
    }
    val dest = File(targetPath)
    dest.parentFile?.mkdirs()
    try {
      val inputStream = BufferedInputStream(assets.open(source))
      val out = BufferedOutputStream(FileOutputStream(dest))
      val buffer = ByteArray(2048)
      var length: Int
      while (true) {
        length = inputStream.read(buffer)
        if (length < 0) {
          break
        }
        out.write(buffer, 0, length)
      }
      out.close()
      inputStream.close()
    } catch (e: FileNotFoundException) {
      e.printStackTrace()
    } catch (e: IOException) {
      e.printStackTrace()
    }
  }

  /**
   * 拷贝assets文件下文件到指定路径
   * @param assetDir  源文件/文件夹
   * @param dir  目标文件夹
   */
  private fun copyAssets(assetDir: String, targetDir: String) {
    if (TextUtils.isEmpty(assetDir) || TextUtils.isEmpty(targetDir)) {
      return
    }
    val separator = File.separator
    try {
      // 获取assets目录assetDir下一级所有文件以及文件夹
      val fileNames = assets.list(assetDir) ?: return
      // 如果是文件夹(目录),则继续递归遍历
      if (fileNames.isNotEmpty()) {
        val targetFile = File(targetDir)
        if (!targetFile.exists() && !targetFile.mkdirs()) {
          return
        }
        for (fileName in fileNames) {
          copyAssets(assetDir + separator + fileName, targetDir + separator + fileName)
        }
      } else { // 文件,则执行拷贝
        copy(assetDir, targetDir)
      }
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }
}