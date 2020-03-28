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
 */

//
// Created by wlanjie on 2019-12-26.
//

#ifndef TRINITY_BLEND_H
#define TRINITY_BLEND_H

#include "opengl.h"
#include "gl.h"
#include "frame_buffer.h"

#define NORMAL_BLEND      0
#define ADD_BLEND         4
#define OVERLAY_BLEND     7
#define ADD2_BLEND        1001
#define COLOR_DODGE_BLEND 1004
#define HARD_LIGHT_BLEND  1009
#define MULTIPLY_BLEND    1015
#define OVERLAY2_BLEND    1017
#define PIN_LIGHT_BLEND   1019
#define SCREEN_BLEND      1021
#define SOFT_LIGHT_BLEND  1022

static const char* BLEND_VERTEX_SHADER =
        "attribute vec3 position;                                                               \n"
        "attribute vec2 inputTextureCoordinate;                                                 \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "uniform mat4 matrix;                                                                   \n"
        "void main() {                                                                          \n"
        "    vec4 p = matrix * vec4(position, 1.);                                              \n"
        "    #ifdef GL_ES                                                                       \n"
        "    textureCoordinate = vec2(inputTextureCoordinate.x, 1.0 - inputTextureCoordinate.y);\n"
        "    #else                                                                              \n"
        "    textureCoordinate = inputTextureCoordinate;                                        \n"
        "    #endif                                                                             \n"
        "    textureCoordinate2 = p.xy * 0.5 + 0.5;                                             \n"
        "    gl_Position = p;                                                                   \n"
        "}                                                                                      \n";

static const char* BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "void main() {                                                                          \n"
        "    vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                   \n"
        "    fgColor = fgColor * alphaFactor;                                                   \n"
        "    gl_FragColor = fgColor;                                                            \n"
        "}                                                                                      \n";

static const char* ADD_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "uniform float bgResolutionWidth;                                                       \n"
        "uniform float bgResolutionHeight;                                                      \n"
        "float blendAdd(float base, float blend) {                                              \n"
        "   return min(base + blend, 1.0);                                                      \n"
        "}                                                                                      \n"
        "vec3 blendAdd(vec3 base, vec3 blend) {                                                 \n"
        "   return min(base + blend, vec3(1.0));                                                \n"
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendAdd(base, blend) * opacity + base * (1.0 - opacity));                  \n"
        "}                                                                                      \n"
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 ); \n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* OVERLAY_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "float blendOverlay(float base, float blend) {                                          \n"
        "   return base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0-blend));\n"
        "}                                                                                      \n"
        "vec3 blendOverlay(vec3 base, vec3 blend) {                                             \n"
        "   return vec3(blendOverlay(base.r, blend.r), blendOverlay(base.g, blend.g), blendOverlay(base.b, blend.b));\n"
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendOverlay(base, blend) * opacity + base * (1.0 - opacity));              \n"
        "}                                                                                      \n"
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* COLOR_DODGE_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "uniform float bgResolutionWidth;                                                       \n"
        "uniform float bgResolutionHeight;                                                      \n"
        "float blendColorDodge(float base, float blend) {                                       \n"
        "   return (blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0);                     \n"
        "}                                                                                      \n"
        "vec3 blendColorDodge(vec3 base, vec3 blend) {                                          \n"
        "   return vec3(blendColorDodge(base.r,blend.r),blendColorDodge(base.g,blend.g),blendColorDodge(base.b,blend.b));\n" // NOLINT
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendColorDodge(base, blend) * opacity + base * (1.0 - opacity));           \n"
        "}                                                                                      \n"
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* HARD_LIGHT_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "float blendHardLight(float base, float blend) {                                        \n"
        "   return blend < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend));\n"
        "}                                                                                      \n"
        "vec3 blendHardLight(vec3 base, vec3 blend) {                                           \n"
        "   return vec3(blendHardLight(base.r,blend.r),blendHardLight(base.g,blend.g),blendHardLight(base.b,blend.b));\n" // NOLINT
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendHardLight(base, blend) * opacity + base * (1.0 - opacity));            \n"
        "}                                                                                      \n"
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* MULTIPLY_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "vec3 blendMultiply(vec3 base, vec3 blend) {                                            \n"
        "   return base * blend;                                                                \n"
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendMultiply(base, blend) * opacity + base * (1.0 - opacity));             \n"
        "}                                                                                      \n"  // NOLINT
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n" // NOLINT
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* PIN_LIGHT_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "float blendDarken(float base, float blend) {                                           \n"
        "   return min(blend, base);                                                            \n"
        "}                                                                                      \n"
        "float blendLighten(float base, float blend) {                                          \n"
        "   return max(blend, base);                                                            \n"
        "}                                                                                      \n" // NOLINT
        "float blendPinLight(float base, float blend) {                                         \n"
        "   return (blend<0.5)?blendDarken(base,(2.0*blend)):blendLighten(base,(2.0*(blend-0.5)));\n"
        "}                                                                                      \n"
        "vec3 blendPinLight(vec3 base, vec3 blend) {                                            \n"
        "   return vec3(blendPinLight(base.r,blend.r),blendPinLight(base.g,blend.g),blendPinLight(base.b,blend.b));\n"
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendPinLight(base, blend) * opacity + base * (1.0 - opacity));             \n"
        "}                                                                                      \n"
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* SCREEN_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "float blendScreen(float base, float blend) {                                           \n"
        "   return 1.0 - ((1.0 - base) * (1.0 - blend));                                        \n"
        "}                                                                                      \n"
        "vec3 blendScreen(vec3 base, vec3 blend) {                                              \n"
        "   return vec3(blendScreen(base.r,blend.r),blendScreen(base.g,blend.g),blendScreen(base.b,blend.b));\n" // NOLINT
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendScreen(base, blend) * opacity + base * (1.0 - opacity));               \n"
        "}                                                                                      \n" // NOLINT
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

static const char* SOFT_LIGHT_BLEND_FRAGMENT_SHADER =
        "#ifdef GL_ES                                                                           \n"
        "precision mediump float;                                                               \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "varying highp vec2 textureCoordinate2;                                                 \n"
        "#else                                                                                  \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "#endif                                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "uniform sampler2D inputImageTexture2;                                                  \n"
        "uniform float alphaFactor;                                                             \n"
        "float blendSoftLight(float base, float blend) {                                        \n"
        "   return (blend<0.5)?(base+(2.0*blend-1.0)*(base-base*base)):(base+(2.0*blend-1.0)*(sqrt(base)-base));\n" // NOLINT
        "}                                                                                      \n"
        "vec3 blendSoftLight(vec3 base, vec3 blend) {                                           \n"
        "   return vec3(blendSoftLight(base.r,blend.r),blendSoftLight(base.g,blend.g),blendSoftLight(base.b,blend.b));\n" // NOLINT
        "}                                                                                      \n"
        "vec3 blendFunc(vec3 base, vec3 blend, float opacity) {                                 \n"
        "   return (blendSoftLight(base, blend) * opacity + base * (1.0 - opacity));            \n"
        "}                                                                                      \n" // NOLINT
        "void main() {                                                                          \n"
        "   vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);                    \n"
        "   fgColor = fgColor * alphaFactor;                                                    \n"
        "   vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);                    \n"
        "   if (fgColor.a == 0.0) {                                                             \n"
        "       gl_FragColor = bgColor;                                                         \n"
        "       return;                                                                         \n"
        "   }                                                                                   \n"
        "   vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), 1.0 );\n"
        "   gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);  \n"
        "}                                                                                      \n";

namespace trinity {

class Blend {
 public:
    explicit Blend(const char* fragment_shader);
    virtual ~Blend();
    int OnDrawFrame(int texture_id, int sticker_texture_id, int width, int height, GLfloat* matrix, float alpha_factor);
 private:
    void CreateFrameBuffer(int width, int height);
    void DeleteFrameBuffer();
    int CreateProgram(const char* vertex, const char* fragment);
    void CompileShader(const char* shader_string, GLuint shader);
    int Link(int program);

 private:
    int program_;
    int second_program_;
    GLuint frame_buffer_id_;
    GLuint frame_buffer_texture_id_;
    GLfloat* default_vertex_coordinates_;
    GLfloat* default_texture_coordinates_;
    int source_width_;
    int source_height_;
    FrameBuffer* frame_buffer_;
};

class NormalBlend : public Blend {
 public:
    NormalBlend() : Blend(BLEND_FRAGMENT_SHADER) {}
};

class AddBlend : public Blend {
 public:
    AddBlend() : Blend(ADD_BLEND_FRAGMENT_SHADER) {}
};

class OverlayBlend : public Blend {
 public:
    OverlayBlend() : Blend(OVERLAY_BLEND_FRAGMENT_SHADER) {}
};

class ColorDodgeBlend : public Blend {
 public:
    ColorDodgeBlend() : Blend(COLOR_DODGE_BLEND_FRAGMENT_SHADER) {}
};

class HardLightBlend : public Blend {
 public:
    HardLightBlend() : Blend(HARD_LIGHT_BLEND_FRAGMENT_SHADER) {}
};

class MultiplyBlend : public Blend {
 public:
    MultiplyBlend() : Blend(MULTIPLY_BLEND_FRAGMENT_SHADER) {}
};

class PinLightBlend : public Blend {
 public:
    PinLightBlend() : Blend(PIN_LIGHT_BLEND_FRAGMENT_SHADER) {}
};

class ScreenBlend : public Blend {
 public:
    ScreenBlend() : Blend(SCREEN_BLEND_FRAGMENT_SHADER) {}
};

class SoftLightBlend : public Blend {
 public:
    SoftLightBlend() : Blend(SOFT_LIGHT_BLEND_FRAGMENT_SHADER) {}
};

class BlendFactory {
 public:
    static Blend* CreateBlend(int blend_type) {
        switch (blend_type) {
            case NORMAL_BLEND:
                return new NormalBlend();
            case ADD_BLEND:
            case ADD2_BLEND:
                return new AddBlend();
            case OVERLAY_BLEND:
            case OVERLAY2_BLEND:
                return new OverlayBlend();
            case COLOR_DODGE_BLEND:
                return new ColorDodgeBlend();
            case HARD_LIGHT_BLEND:
                return new HardLightBlend();
            case MULTIPLY_BLEND:
                return new MultiplyBlend();
            case PIN_LIGHT_BLEND:
                return new PinLightBlend();
            case SCREEN_BLEND:
                return new ScreenBlend();
            case SOFT_LIGHT_BLEND:
                return new SoftLightBlend();
            default:
                return new NormalBlend();
        }
        return nullptr;
    }
};

}  // namespace trinity

#endif  // TRINITY_BLEND_H
