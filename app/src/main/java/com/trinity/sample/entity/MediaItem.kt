package com.trinity.sample.entity

import java.io.Serializable

class MediaItem(
    val path: String,
    val type: String,
    val width: Int,
    val height: Int,
    val duration: Int
) : Serializable