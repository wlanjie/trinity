# trinity
![icon~](https://github.com/wlanjie/trinity/blob/master/trinity@2x.png)

[![Build Status](https://travis-ci.org/LaiFengiOS/LFLiveKit.svg)](https://travis-ci.org/LaiFengiOS/LFLiveKit)&nbsp;
![Version](https://img.shields.io/badge/version-v1.0-blue.svg)&nbsp;
![platform](https://img.shields.io/badge/platform-Android-orange.svg)&nbsp;
<!-- [![License MIT](https://img.shields.io/badge/license-MIT-green.svg?style=flat)](https://raw.githubusercontent.com/chenliming777/LFLiveKit/master/LICENSE)&nbsp;
[![CocoaPods](http://img.shields.io/cocoapods/v/LFLiveKit.svg?style=flat)](http://cocoapods.org/?q=LFLiveKit)&nbsp; -->


**trinity是一个开源的拍摄和短视频处理工具，用kotlin和c++编写，实现了大部分短视频编辑软件热门功能。**

## 功能
- [x] 	有声短视频拍摄
- [x] 	支持横屏和竖屏拍摄
- [x] 	支持自定义相机配置
- [x] 	支持自定义麦克风配置
- [x] 	支持导出视频参数自定义
- [x] 	导入短视频编辑
- [x] 	分段添加滤镜功能
- [x] 	分段添加视频特效功能
- [x] 	添加背景音乐

## 使用

### 录制

创建预览视图

```
val preview = findViewById<TrinityPreviewView>(R.id.preview)
```

配置相机

```
val videoConfiguration = VideoConfiguration()
```

创建录制实例

```
mRecord = TrinityRecord(preview, videoConfiguration)
mRecord.setOnRenderListener(this)
```

开启录制

```
mRecord.startEncode("/sdcard/a.mp4",         //save path
                      720,                   //video width
                      1280,                  //video height
                      720 * 1280 * 20,       //video bitrate
                      30,                    //video fps
                      false,                 //use hardware encode
                      44100,                 //audio samplerate
                      1,                     //audio channel
                      128 * 1000,            //audio bitrate
                      Int.MAX_VALUE)         //record max duration
```

结束录制

```
mRecord.stopEncode()
```

### 视频编辑

创建编辑实例

```
mVideoEditor = VideoEditor()
```

设置预览画面

```
val surfaceView = findViewById<SurfaceView>(R.id.surface_view)
mVideoEditor.setSurfaceView(surfaceView)
```

打开视频文件

```
val editorFile = externalCacheDir?.absolutePath + "/editor.mp4"
val file = File(editorFile)
if (!file.exists()) {
    val stream = assets.open("editor.mp4")
    val outputStream = FileOutputStream(file)
    val buffer = ByteArray(2048)
    while (true) {
    val length = stream.read(buffer)
    if (length == -1) {
        break
    }
    outputStream.write(buffer)
    }
}
```

添加视频片段

```
val clip = MediaClip(file.absolutePath)
mVideoEditor.insertClip(clip)
```

> 添加多个视频片段需要打开多个视频文件 创建相应视频片段实例并插入

```
val editorFileSecond = externalCacheDir?.absolutePath + "/editor_second.mp4"
val fileSecond = File(editorFileSecond)
if (!fileSecond.exists()) {
    val stream = assets.open("editor_second.mp4")
    val outputStream = FileOutputStream(fileSecond)
    val buffer = ByteArray(2048)
    while (true) {
    val length = stream.read(buffer)
    if (length == -1) {
        break
    }
    outputStream.write(buffer)
    }
}

val clipSecond = MediaClip(fileSecond.absolutePath)
mVideoEditor.insertClip(clipSecond)
```

开始播放

```
mVideoEditor.play(true)     //is repeat
```

添加滤镜

```
mVideoEditor.addFilter(buffer.array(),   //色阶图buffer
                       0,                //startTime
                       Long.MAX_VALUE)   //endTime
```

添加背景音乐

```
mVideoEditor.addFilter("background/music.mp3",   //music path
                       0,                        //startTime
                       Long.MAX_VALUE)           //endTime
```

导出视频

```
val config = assets.open("export_config.json").bufferedReader().use { it.readText() }
mVideoEditor.export(config,                     //config
                    "/sdcard/export.mp4",       //path
                    540,                        //video width
                    960,                        //video height
                    25,                         //video fps
                    3000 * 1024,                //video bitrate
                    44100,                      //audio samplerate
                    1,                          //audio channel
                    128 * 1000)                 //audio bitrate
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































