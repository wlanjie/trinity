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
// Created by wlanjie on 2019-05-26.
//

#include "pixel_late.h"
#include "gl.h"

namespace trinity {

const char* PIXEL_LATE_FRAGMENT = "precision mediump float;\n"
                                  "varying vec2 textureCoordinate;\n"
                                  "uniform sampler2D inputImageTexture;\n"
                                  "uniform float fractionalWidthOfPixel;\n"
                                  "uniform float aspectRatio;\n"
                                  "void main() {\n"
                                  "//    vec2 sampleDivisor = vec2(fractionalWidthOfPixel, fractionalWidthOfPixel / aspectRatio);\n"
                                  "//    vec2 samplePos = textureCoordinate - (textureCoordinate - sampleDivisor * (floor(textureCoordinate / sampleDivisor))) + 0.5 * sampleDivisor;\n"
                                  "//    vec2 samplePos = textureCoordinate - mod(textureCoordinate, sampleDivisor) + 0.5 * sampleDivisor;\n"
                                  "//    gl_FragColor = texture2D(inputImageTexture, samplePos);\n"
                                  "//     https://www.jianshu.com/p/7e82d08dc1db\n"
                                  "//     要实现马赛克的效果，需要把图片一个相当大小的正方形区域用同一个点的颜色来表示，相当于将连续的颜色离散化，因此我们可以想到用取整的方法来离散颜色，但是在我们的图片纹理坐标采样时在0到1的连续范围，因此我们需要将纹理坐标转换成实际大小的整数坐标，接下来要把图像这个坐标量化离散，比如对于一个分辨率为256X256像素的图片，马赛克块的大小为8X8像素，我们先得将纹理坐标乘以（256,256）使其映射到0到256的范围，相当于一个整数代表一个像素，再将纹理坐标取除以8之后取整，最后再将坐标乘以8，除以256.重新映射回0到1的范围，但是纹理坐标已经是离散的，而非连续的\n"
                                  "//    vec2 mosaicSize = vec2(28.0, 28.0);\n"
                                  "//    vec2 size = vec2(676.0, 1155.0);\n"
                                  "//    vec2 xy = vec2(textureCoordinate.x * size.x, textureCoordinate.y * size.y);\n"
                                  "//    vec2 mosaic = vec2(floor(xy.x / mosaicSize.x) * mosaicSize.x, floor(xy.y / mosaicSize.y) * mosaicSize.y);\n"
                                  "//    gl_FragColor = texture2D(inputImageTexture, vec2(mosaic.x / size.x, mosaic.y / size.y));\n"
                                  "//    vec2 mosaicSize = vec2(28.0, 28.0);\n"
                                  "//    vec2 size = vec2(676.0, 1155.0);\n"
                                  "//    vec2 xy = textureCoordinate * size;\n"
                                  "//    vec2 mosaic = vec2(floor(xy / mosaicSize) * mosaicSize);\n"
                                  "//    vec2 mosaic = vec2(floor(xy.x / mosaicSize.x) * mosaicSize.x, floor(xy.y / mosaicSize.y) * mosaicSize.y);\n"
                                  "//    gl_FragColor = texture2D(inputImageTexture, mosaic / size);\n"
                                  "    float dx = 14.0 * (1.0 / 720.0);\n"
                                  "    float dy = 14.0 * (1.0 / 1280.0);\n"
                                  "    vec2 coord = vec2(dx * floor(textureCoordinate.x / dx), dy * floor(textureCoordinate.y / dy));\n"
                                  "    gl_FragColor = texture2D(inputImageTexture, coord);\n"
                                  "}\n";

#define PIXEL 0.05

PixelLate::PixelLate(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, PIXEL_LATE_FRAGMENT) {
    this->width_ = width;
    this->height_ = height;
}

PixelLate::~PixelLate() {

}

void PixelLate::RunOnDrawTasks() {
    FrameBuffer::RunOnDrawTasks();
    float singlePixelSpacing = 0;
    if (width_ != 0) {
        singlePixelSpacing = 1.0f / width_;
    } else {
        singlePixelSpacing = 1.0f / 2048;
    }

    if (singlePixelSpacing < PIXEL) {
        singlePixelSpacing = PIXEL;
    } else {
//        singlePixelSpacing = newValue;
    }
    SetFloat("fractionalWidthOfPixel", singlePixelSpacing);
    SetFloat("aspectRatio", height_ * 1.0f / width_);
//    setFloat("aspectRatio", width * 1.0f / height);
}

void PixelLate::OnDrawArrays() {

}

GLuint PixelLate::OnDrawFrame(int textureId) {
    return FrameBuffer::OnDrawFrame(textureId);
}

}
