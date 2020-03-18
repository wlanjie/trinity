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
// Created by wlanjie on 2019-06-11.
//

#ifndef TRINITY_FILTER_H
#define TRINITY_FILTER_H

#include "frame_buffer.h"
#include "gl.h"

namespace trinity {

static const char* FILTER_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                                \n"
        "precision highp float;                                                                      \n"
        "varying highp vec2 textureCoordinate;                                                       \n"
        "varying highp vec2 textureCoordinate2;                                                      \n"
        "#else                                                                                       \n"
        "varying vec2 textureCoordinate;                                                             \n"
        "varying vec2 textureCoordinate2;                                                            \n"
        "#endif                                                                                      \n"
        "uniform sampler2D inputImageTexture;                                                        \n"
        "uniform sampler2D inputImageTextureLookup;                                                  \n"
        "uniform float intensity;                                                                    \n"
        "void main () {                                                                              \n"
        " vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);                       \n"
        " if (textureCoordinate.y < 1.0) {                                                           \n"
        "     float yColor = textureColor.b * 63.0;                                                  \n"
        "     vec2 quad1;                                                                            \n"
        "     quad1.y = floor(floor(yColor) / 8.0);                                                  \n"
        "     quad1.x = floor(yColor) - (quad1.y * 8.0);                                             \n"
        "     vec2 quad2;                                                                            \n"
        "     quad2.y = floor(ceil(yColor) / 8.0);                                                   \n"
        "     quad2.x = ceil(yColor) - (quad2.y * 8.0);                                              \n"
        "     vec2 texPos1;                                                                          \n"
        "     texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);    \n"
        "     texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);    \n"
        "     vec2 texPos2;                                                                          \n"
        "     texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);    \n"
        "     texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);    \n"
        "     vec4 newColor1;                                                                        \n"
        "     vec4 newColor2;                                                                        \n"
        "     newColor1 = texture2D(inputImageTextureLookup, texPos1);                               \n"
        "     newColor2 = texture2D(inputImageTextureLookup, texPos2);                               \n"
        "     vec4 newColor = mix(newColor1, newColor2, fract(yColor));                              \n"
        "     gl_FragColor = mix(textureColor, vec4(newColor.rgb, textureColor.a), intensity);       \n"
        " } else {                                                                                   \n"
        "     gl_FragColor = textureColor;                                                           \n"
        " }                                                                                          \n"
        "}\n";

static const char* FILTER_VERTEX_SHADER =
        "#ifdef GL_ES                                                                                \n"
        "precision highp float;                                                                      \n"
        "#endif                                                                                      \n"
        "attribute vec4 position;                                                                    \n"
        "attribute vec2 inputTextureCoordinate;                                                      \n"
        "varying vec2 textureCoordinate;                                                             \n"
        "varying vec2 textureCoordinate2;                                                            \n"
        "void main() {                                                                               \n"
        "    gl_Position = position;                                                                 \n"
        "    textureCoordinate = inputTextureCoordinate.xy;                                          \n"
        "    textureCoordinate2 = inputTextureCoordinate.xy * 0.5 + 0.5;                             \n"
        "}                                                                                           \n";


class Filter : public FrameBuffer {
 public:
    Filter(uint8_t* filter_buffer, int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, FILTER_FRAGMENT_SHADER) {
        intensity_ = 1.0f;
        filter_texture_id_ = 0;
        glGenTextures(1, &filter_texture_id_);
        glBindTexture(GL_TEXTURE_2D, filter_texture_id_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, filter_buffer);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void UpdateLut(uint8_t* lut, int width, int height) {
        glBindTexture(GL_TEXTURE_2D, filter_texture_id_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, lut);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void SetIntensity(float intensity = 1.0f) {
        intensity_ = intensity;
    }

    ~Filter() {
        if (filter_texture_id_ != 0) {
            glDeleteTextures(1, &filter_texture_id_);
            filter_texture_id_ = 0;
        }
    }

 private:
    GLuint filter_texture_id_;
    float intensity_;

 protected:
    virtual void RunOnDrawTasks() {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, filter_texture_id_);
        SetInt("inputImageTextureLookup", 1);
        SetFloat("intensity", intensity_);
    }
};

}  // namespace trinity

#endif  // TRINITY_FILTER_H
