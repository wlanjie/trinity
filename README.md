# trinity
![icon~](https://github.com/wlanjie/trinity/blob/master/trinity@2x.png)

[![Build Status](https://travis-ci.org/LaiFengiOS/LFLiveKit.svg)](https://travis-ci.org/LaiFengiOS/LFLiveKit)&nbsp;
![Version](https://img.shields.io/badge/version-v1.0-blue.svg)&nbsp;
![platform](https://img.shields.io/badge/platform-Android-orange.svg)&nbsp;
<!-- [![License MIT](https://img.shields.io/badge/license-MIT-green.svg?style=flat)](https://raw.githubusercontent.com/chenliming777/LFLiveKit/master/LICENSE)&nbsp; -->


**trinity是一个开源的拍摄和短视频处理工具，用kotlin和c++编写，实现了大部分短视频编辑软件热门功能。**

[apk下载](https://github.com/wlanjie/trinity/blob/master/trinity.apk)

![演示](screen_shot.gif)

## 系统版本
支持Android 4.3及以上版本

## 开发环境
- Android Studio 3.5
- NDK r20
- kotlin 1.3.41

## 使用的开源库
- fdk-aac
- ffmpeg 3.4
- libx264
- xlogger

## 功能
- [x] 	有声短视频拍摄
- [x] 	支持横屏和竖屏拍摄
- [x] 	支持自定义相机配置
- [x]   支持快慢速拍摄
- [x] 	支持自定义麦克风配置
- [x] 	支持导出视频参数自定义
- [x] 	导入短视频编辑
- [x] 	分段添加滤镜功能
- [x] 	分段添加视频特效功能
- [x] 	添加背景音乐

## 使用

<font color=red>注意: SDK中不做权限判断,使用时需要由调用方申请好权限, SDK中涉及到的时间均为毫秒</font>

### 权限要求
``` xml
<uses-permission android:name="android.permission.CAMERA" />
<uses-permission android:name="android.permission.RECORD_AUDIO" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
```

## 录制

### 配置参数
- 创建录制预览view
``` kotlin
val preview = findViewById<TrinityPreviewView>(R.id.preview)
```

- 创建录制接口实例
``` kotlin
mRecord = TrinityRecord(preview)
```

- 销毁录制接口实例
``` kotlin
mRecord.release()
```

### 回调设置
- 设置视频渲染回调
``` kotlin
mRecord.setOnRenderListener(this)
```

- 设置录制进度回调
``` kotlin
mRecord.setOnRecordingListener(this)
```

- 设置相机回调
``` kotlin
mRecord.setCameraCallback(this)
```

### 开启预览
- 开始预览
``` kotlin
mRecord.startPreview()
```

- 结束预览
``` kotlin
mRecord.stopPreview()
```

- 设置预览类型
``` kotlin
// 设置显示类型
// 包含裁剪显示, 原比例上下留黑显示
mRecord.setFrame(mFrame)
```

### 录制控制/管理
- 切换摄像头
``` kotlin
mRecord.switchCamera()
```

- 获取当前摄像头
``` kotlin
// 返回当前摄像头id
val facing = mRecord.getCameraFacing()
```

- 开关闪光灯
``` kotlin
mRecord.flash(mFlash)
```
- 设置zoom
``` kotlin
// 设置焦距缩放, 0-100 100为最大缩放
mRecord.setZoom(0)
```
- 设置曝光度
``` kotlin
// 设置相机曝光度, 100为最大曝光
mRecord.setExposureCompensation(0)
```
- 手动对焦
``` kotlin
// 设置手动对焦, 参数为x和y
mRecord.focus(mPointF)
```
- 设置录制视频的角度
``` kotlin
/**
 * @param rotation 旋转角度包含 0 90 180 270
 */
mRecord.setRecordRotation(0)
```
- 设置静音录制
``` kotlin
mRecord.setMute(false)
```
- 倍速录制
``` kotlin
/**
 * @param speed 速度包含 0.25 0.5 1.0 2.0 4.0倍速
 */
mRecord.setSpeed(mSpeed)
```

### 添加特效
- 添加滤镜
``` kotlin
/**
 * @param config 滤镜json内容
 * 具体json内容如下:
 * {
 *    "effectType": "Filter",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "lut": "/sdcard/lut.png",
 *    "intensity": 1.0
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, Filter为添加一个普通滤镜效果
 * startTime: 这个滤镜的开始时间
 * endTime: 这个滤镜的结束时间 2000代表这个滤镜在显示时只显示2秒钟
 * lut: 滤镜颜色表的地址, 必须为本地地址
 * intensity: 滤镜透明度, 0.0时和摄像头采集的无差别
 */
val actionId = mRecord.addAction(config)
```

### 开始录制

- 开始录制一段视频
``` kotlin
/**
 * 开始录制一段视频
 * @param path 录制的视频保存的地址
 * @param width 录制视频的宽, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
 * @param height 录制视频的高, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
 * @param videoBitRate 视频输出的码率, 如果设置的是2000, 则为2M, 最终输出的和设置的可能有差别
 * @param frameRate 视频输出的帧率
 * @param useHardWareEncode 是否使用硬编码, 如果设置为true, 而硬编码不支持,则自动切换到软编码
 * @param audioSampleRate 音频的采样率
 * @param audioChannel 音频的声道数
 * @param audioBitRate 音频的码率
 * @param duration 需要录制多少时间
 * @return Int ErrorCode.SUCCESS 为成功,其它为失败
 * @throws InitRecorderFailException
 */
mRecord.startRecording("/sdcard/a.mp4",
                      720,
                      1280,
                      2000, // 2M码率
                      30,
                      false,
                      44100,
                      1, // 单声道
                      128, // 128K 码率
                      Int.MAX_VALUE)
```

- 结束录制
``` kotlin
mRecord.stopRecording()
```

### 视频编辑

####初始化

- 创建编辑器实例
``` kotlin
mVideoEditor = TrinityCore.createEditor(this)
```

- 设置预览画面
``` kotlin
val surfaceView = findViewById<SurfaceView>(R.id.surface_view)
mVideoEditor.setSurfaceView(surfaceView)
```

#### 导入视频
- 添加一个片段
``` kotlin
val clip = MediaClip(file.absolutePath)
mVideoEditor.insertClip(clip)
```
- 根据下标添加片段
``` kotlin
val clip = MediaClip(file.absolutePath)
mVideoEditor.insertClip(0, clip)
```
- 删除一个片段
``` kotlin
/**
 * 根据下标删除一个片段
 */
mVideoEditor.removeClip(index)
```
- 获取片段的数量
``` kotlin
val count = mVideoEditor.getClipsCount()
```
- 根据下标获取一个片段
``` kotlin
/**
 * 如果片段不存在, 返回一个null
 */
val clip = mVideoEditor.getClip(index)
```
- 根据下标替换一个片段
``` kotlin
mVideoEditor.replaceClip(index, clip)
```
- 获取所有片段
``` kotlin
/**
 * 返回所有片段的集合
 */
val clips = mVideoEditor.getVideoClips()
```
- 获取所有片段的时间总长
``` kotlin
val duration = mVideoEditor.getVideoDuration()
```
- 获取当前播放片段的进度
``` kotlin
val current = mVideoEditor.getCurrentPosition()
```
- 获取指定片段的开始和结束时间
``` kotlin
val timeRange = mVideoEditor.getClipTimeRange(index)
```
- 根据时间查找片段的下标
``` kotlin
val index = mVideoEditor.getClipIndex(time)
```
#### 背景音乐
- 添加背景音乐
``` kotlin
/**
 * @param config 背景音乐json内容
 * 具体json内容如下:
 * {
 *    "path": "/sdcard/trinity.mp3",
 *    "startTime": 0,
 *    "endTime": 2000
 * }
 * json 参数解释:
 * path: 音乐的本地地址
 * startTime: 这个音乐的开始时间
 * endTime: 这个音乐的结束时间 2000代表这个音乐只播放2秒钟
 */
val actionId = mVideoEditor.addMusic(config)
```

- 更新背景音乐
``` kotlin
/**
 * @param config 背景音乐json内容
 * 具体json内容如下:
 * {
 *    "path": "/sdcard/trinity.mp3",
 *    "startTime": 2000,
 *    "endTime": 4000
 * }
 * json 参数解释:
 * path: 音乐的本地地址
 * startTime: 这个音乐的开始时间
 * endTime: 这个音乐的结束时间 4000代表这个音乐从开始时间到结束时间播放2秒钟
 */
val actionId = mVideoEditor.addMusic(config)
```

- 删除背景音乐
``` kotlin
mVideoEditor.deleteMusic(actionId)
```

#### 添加特效
- 添加普通滤镜
``` kotlin
/**
 * @param config 滤镜json内容
 * 具体json内容如下:
 * {
 *    "effectType": "Filter",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "lut": "/sdcard/lut.png",
 *    "intensity": 1.0
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, Filter为添加一个普通滤镜效果
 * startTime: 这个滤镜的开始时间
 * endTime: 这个滤镜的结束时间 2000代表这个滤镜在显示时只显示2秒钟
 * lut: 滤镜颜色表的地址, 必须为本地地址
 * intensity: 滤镜透明度, 0.0时和摄像头采集的无差别
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加闪白效果
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "FlashWhite",
 *    "startTime": 0,
 *    "endTime": 2000
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, FlashWhite为添加一个闪白效果
 * startTime: 这个闪白的开始时间
 * endTime: 这个闪白的结束时间 2000代表这个闪白效果在显示时只显示2秒钟
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加两分屏
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "SplitScreen",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "splitScreenCount": 2
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, SplitScreen为添加一个分屏效果
 * startTime: 这个分屏的开始时间
 * endTime: 这个分屏的结束时间 2000代表这个分屏效果在显示时只显示2秒钟
 * splitScreenCount: 分屏数量2个
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加三分屏
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "SplitScreen",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "splitScreenCount": 3
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, SplitScreen为添加一个分屏效果
 * startTime: 这个分屏的开始时间
 * endTime: 这个分屏的结束时间 2000代表这个分屏效果在显示时只显示2秒钟
 * splitScreenCount: 分屏数量3个
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加四分屏
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "SplitScreen",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "splitScreenCount": 4
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, SplitScreen为添加一个分屏效果
 * startTime: 这个分屏的开始时间
 * endTime: 这个分屏的结束时间 2000代表这个分屏效果在显示时只显示2秒钟
 * splitScreenCount: 分屏数量4个
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加六分屏
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "SplitScreen",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "splitScreenCount": 6
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, SplitScreen为添加一个分屏效果
 * startTime: 这个分屏的开始时间
 * endTime: 这个分屏的结束时间 2000代表这个分屏效果在显示时只显示2秒钟
 * splitScreenCount: 分屏数量6个
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加九分屏
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "SplitScreen",
 *    "startTime": 0,
 *    "endTime": 2000,
 *    "splitScreenCount": 9
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, SplitScreen为添加一个分屏效果
 * startTime: 这个分屏的开始时间
 * endTime: 这个分屏的结束时间 2000代表这个分屏效果在显示时只显示2秒钟
 * splitScreenCount: 分屏数量9个
 */
val actionId = mVideoEditor.addAction(config)
```
- 添加模糊分屏
``` kotlin
/**
 * @param config 闪白json内容
 * 具体json内容如下:
 * {
 *    "effectType": "BlurSplitScreen",
 *    "startTime": 0,
 *    "endTime": 2000
 * }
 *
 * json 参数解释:
 * effectType: 底层根据这个字段来判断添加什么效果, BlurSplitScreen为添加一个模糊分屏效果
 * startTime: 这个模糊分屏的开始时间
 * endTime: 这个模糊分屏的结束时间 2000代表这个模糊分屏效果在显示时只显示2秒钟
 */
val actionId = mVideoEditor.addAction(config)
```

#### 开始预览
- 播放
``` kotlin
/**
 * @param repeat 是否循环播放
 */
mVideoEditor.play(repeat)
```
- 暂停
``` kotlin
mVideoEditor.pause()
```
- 继续播放
``` kotlin
mVideoEditor.resume()
```
- 停止播放
``` kotlin
mVideoEditor.stop()
```

#### 释放资源
``` kotlin
mVideoEditor.destroy()
```

### 导出视频
- 创建导出实例
``` kotlin
val export = TrinityCore.createExport(this)
```
- 开始导出
``` kotlin
/**
  * 开始导出
  * @param path 录制的视频保存的地址
  * @param width 录制视频的宽, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
  * @param height 录制视频的高, SDK中会做16倍整数的运算, 可能最终输出视频的宽和设置进去的不一致
  * @param videoBitRate 视频输出的码率, 如果设置的是2000, 则为2M, 最终输出的和设置的可能有差别
  * @param frameRate 视频输出的帧率
  * @param sampleRate 音频的采样率
  * @param channelCount 音频的声道数
  * @param audioBitRate 音频的码率
  * @param l 导出回调 包含成功 失败 和进度回调
  * @return Int ErrorCode.SUCCESS 为成功,其它为失败
  */
export.export("/sdcard/export.mp4",
              544,
              960,
              25,
              3000,
              44100,
              1,
              128,
              this)
```
- 取消
``` kotlin
export.cancel()
```
- 释放
``` kotlin
export.release()
```

```
Copyright 2019 Trinity, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
