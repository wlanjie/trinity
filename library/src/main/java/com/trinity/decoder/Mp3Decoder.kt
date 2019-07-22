package com.trinity.decoder

interface Mp3Decoder {

    fun init(packetBufferTimePercent: Float, vocalSampleRate: Int)
    fun setAccompanyVolume(volume: Float, accompanyMax: Float)
    fun destroy()
    fun readSamples(samples: ShortArray, slientSizeArr: IntArray, extraSampleType: IntArray): Int
    fun startAccompany(path: String)
    fun pauseAccompany()
    fun resumeAccompany()
    fun stopAccompany()
}
