//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_GL_H
#define TRINITY_GL_H

static const char* DEFAULT_VERTEX_MATRIX_SHADER =
        "attribute vec4 position;\n"
        "attribute vec4 inputTextureCoordinate;\n"
        "varying vec2 textureCoordinate;\n"
        "uniform highp mat4 textureMatrix; \n"
        "void main() {\n"
        "  textureCoordinate = (textureMatrix * inputTextureCoordinate).xy;\n"
        "  gl_Position = position;\n"
        "}\n";

static const char* DEFAULT_OES_FRAGMENT_SHADER =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES yuvTexSampler;\n"
        "varying vec2 textureCoordinate;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(yuvTexSampler, textureCoordinate);\n"
        "}\n";

static const char* DEFAULT_VERTEX_SHADER =
        "precision highp float;\n"
        "attribute vec4 position;\n"
        "attribute vec4 inputTextureCoordinate;\n"
        "varying vec2 textureCoordinate;\n"
        "void main() {\n"
        "    gl_Position = position;\n"
        "    textureCoordinate = inputTextureCoordinate.xy;\n"
        "}";

static const char* DEFAULT_FRAGMENT_SHADER =
        "precision highp float;\n"
        "varying vec2 textureCoordinate;\n"
        "uniform sampler2D inputImageTexture;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(inputImageTexture, textureCoordinate);\n"
        "}";

static const char* FLASH_WHITE_FRAGMENT_SHADER =
        "precision highp float;\n"
        "varying vec2 textureCoordinate;\n"
        "uniform sampler2D inputImageTexture;\n"
        "uniform float exposureColor;\n"
        "void main() {\n"
        "    vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n"
        "    gl_FragColor = vec4(textureColor.r + exposureColor, textureColor.g + exposureColor, textureColor.b + exposureColor, textureColor.a);\n"
        "}\n";

static const char *SCREEN_TWO_FRAGMENT_SHADER =
        "precision highp float;                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "void main() {                                                                          \n"
        "    int col = int(textureCoordinate.y * 2.0);                                          \n"
        "    vec2 textureCoordinateToUse = textureCoordinate;                                   \n"
        "    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 2.0) * 2.0;         \n"
        "    textureCoordinateToUse.y = textureCoordinateToUse.y/960.0*480.0+1.0/4.0;           \n"
        "    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);                 \n"
        "}                                                                                      \n";

static const char* SCREEN_THREE_FRAGMENT_SHADER =
        "precision highp float;                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "void main() {                                                                          \n"
        "    int col = int(textureCoordinate.y * 3.0);                                          \n"
        "    vec2 textureCoordinateToUse = textureCoordinate;                                   \n"
        "    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 3.0) * 3.0;         \n"
        "    textureCoordinateToUse.y = textureCoordinateToUse.y/960.0*320.0+1.0/3.0;           \n"
        "    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);                 \n"
        "}                                                                                      \n";

static const char* SCREEN_FOUR_FRAGMENT_SHADER =
        "precision highp float;                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "void main() {                                                                          \n"
        "    int row = int(textureCoordinate.x * 2.0);                                          \n"
        "    int col = int(textureCoordinate.y * 2.0);                                          \n"
        "    vec2 textureCoordinateToUse = textureCoordinate;                                   \n"
        "    textureCoordinateToUse.x = (textureCoordinate.x - float(row) / 2.0) * 2.0;         \n"
        "    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 2.0) * 2.0;         \n"
        "    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);                 \n"
        "}                                                                                      \n";

static const char* SCREEN_SIX_FRAGMENT_SHADER =
        "precision highp float;                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "void main() {                                                                          \n"
        "    int row = int(textureCoordinate.x * 3.0);                                          \n"
        "    int col = int(textureCoordinate.y * 2.0);                                          \n"
        "    vec2 textureCoordinateToUse = textureCoordinate;                                   \n"
        "    textureCoordinateToUse.x = (textureCoordinate.x - float(row) / 3.0) * 3.0;         \n"
        "    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 2.0) * 2.0;         \n"
        "    textureCoordinateToUse.x = textureCoordinateToUse.x/540.0*360.0+90.0/540.0;        \n"
        "    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);                 \n"
        "}                                                                                      \n";

static const char* SCREEN_NINE_FRAGMENT_SHADER =
        "precision highp float;                                                                 \n"
        "uniform sampler2D inputImageTexture;                                                   \n"
        "varying highp vec2 textureCoordinate;                                                  \n"
        "void main() {                                                                          \n"
        "    int row = int(textureCoordinate.x * 3.0);                                          \n"
        "    int col = int(textureCoordinate.y * 3.0);                                          \n"
        "    vec2 textureCoordinateToUse = textureCoordinate;                                   \n"
        "    textureCoordinateToUse.x = (textureCoordinate.x - float(row) / 3.0) * 3.0;         \n"
        "    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 3.0) * 3.0;         \n"
        "    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);                 \n"
        "}                                                                                      \n";
#endif //TRINITY_GL_H
