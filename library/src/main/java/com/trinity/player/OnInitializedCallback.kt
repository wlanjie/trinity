package com.trinity.player

interface OnInitializedCallback {

  enum class OnInitialStatus {
    CONNECT_SUCESS,
    CONNECT_FAILED,
    CLINET_CANCEL
  }

  fun onInitialized(onInitialStatus: OnInitialStatus)
}
