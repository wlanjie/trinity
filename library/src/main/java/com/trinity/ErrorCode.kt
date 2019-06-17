package com.trinity

/**
 * Create by wlanjie on 2019/4/15-下午8:36
 */
object ErrorCode {

  // 0成功
  const val SUCCESS = 0

  // 公共的 从-500开始
  const val FILE_NOT_FOUND = -500

  // Record -1000 开始

  // 正在录制中
  const val RECORDING = -1001
  const val FILE_NOT_EXISTS = -1002

  // 编辑模块 -2000开始
}