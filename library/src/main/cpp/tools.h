/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_TOOLS_H
#define TRINITY_TOOLS_H

#include <sys/time.h>

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

static inline long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline long getCurrentTimeMills() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline long getCurrentTimeSeconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

static inline long long currentTimeMills(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 合并两个short，返回一个short
inline short TPMixSamples(short a, short b) {
    int tmp = a < 0 && b < 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MIN) 
        : (a > 0 && b > 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MAX) : a + b);
    return tmp > INT16_MAX ? INT16_MAX : (tmp < INT16_MIN ? INT16_MIN : tmp);
}

// 合并两个float，返回一个short
inline short TPMixSamplesFloat(float a, float b) {
    int tmp = a < 0 && b < 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MIN) 
        : (a > 0 && b > 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MAX) : a + b);
    return tmp > INT16_MAX ? INT16_MAX : (tmp < INT16_MIN ? INT16_MIN : tmp);
}

// 把一个short转换为一个长度为2的byte数组
inline void converttobytearray(short source, uint8_t * bytes2) {
    bytes2[0] = (uint8_t) (source & 0xff);
    bytes2[1] = (uint8_t) ((source >> 8) & 0xff);
}

// 将两个byte转换为一个short
inline short convertshort(uint8_t* bytes) {
    return (bytes[0] << 8) + (bytes[1] & 0xFF);
}

// 调节音量的方法
inline short adjustAudioVolume(short source, float volume) {
    short result = source;
    int temp = static_cast<int>((static_cast<int>(source) * volume));
    if (temp < -0x8000) {
        result = -0x8000;
    } else if (temp > 0x7FFF) {
        result = 0x7FFF;
    } else {
        result = (short) temp;
    }
    return result;
}

// 调节采样的音量---非清唱时候最终合成伴奏与录音的时候，判断如果accompanyVolume不是1.0的话，先调节伴奏的音量；而audioVolume是在读出的字节流转换为short流的时候调节的。
inline void adjustSamplesVolume(short *samples, int size, float accompanyVolume) {
    if (accompanyVolume != 1.0) {
        for (int i = 0; i < size; i++) {
            samples[i] = adjustAudioVolume(samples[i], accompanyVolume);
        }
    }
}

// 合成伴奏与录音,byte数组需要在客户端分配好内存---非清唱时候最终合成伴奏与录音调用
inline void mixtureAccompanyAudio(short *accompanyData, short *audioData, int size, uint8_t *targetArray) {
    uint8_t* tmpbytearray = new uint8_t[2];
    for (int i = 0; i < size; i++) {
        short audio = audioData[i];
        short accompany = accompanyData[i];
        short temp = TPMixSamples(accompany, audio);
        converttobytearray(temp, tmpbytearray);
        targetArray[i * 2] = tmpbytearray[0];
        targetArray[i * 2 + 1] = tmpbytearray[1];
    }
    delete[] tmpbytearray;
}

// 合成伴奏与录音,short数组需要在客户端分配好内存---非清唱时候边和边放调用
inline void mixtureAccompanyAudio(short *accompanyData, short *audioData, int size, short *targetArray) {
    for (int i = 0; i < size; i++) {
        short audio = audioData[i];
        short accompany = accompanyData[i];
        targetArray[i] = TPMixSamples(accompany, audio);
    }
}

// 将一个byte数组转换为一个short数组---读进来audioRecord录制的pcm，然后将其从文件中读出字节流，然后在转换为short以方便后续处理
inline void convertShortArrayFromByteArray(uint8_t *bytearray, int size, short *shortarray, float audioVolume) {
    uint8_t* bytes = new uint8_t[2];
    for (int i = 0; i < size / 2; i++) {
        bytes[1] = bytearray[2 * i];
        bytes[0] = bytearray[2 * i + 1];
        short source = convertshort(bytes);
        if (audioVolume != 1.0) {
            shortarray[i] = adjustAudioVolume(source, audioVolume);
        } else {
            shortarray[i] = source;
        }
    }
    delete[] bytes;
}

#endif  // TRINITY_TOOLS_H
