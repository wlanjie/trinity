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
        "attribute vec4 position;\n"
        "attribute vec4 inputTextureCoordinate;\n"
        "varying vec2 textureCoordinate;\n"
        "void main() {\n"
        "    gl_Position = position;\n"
        "    textureCoordinate = inputTextureCoordinate.xy;\n"
        "}";

static const char* DEFAULT_FRAGMENT_SHADER =
        "precision mediump float;\n"
        "varying vec2 textureCoordinate;\n"
        "uniform sampler2D inputImageTexture;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(inputImageTexture, textureCoordinate);\n"
        "}";

static const char* FLASH_WHITE_FRAGMENT_SHADER =
        "precision mediump float;\n"
        "varying vec2 textureCoordinate;\n"
        "uniform sampler2D inputImageTexture;\n"
        "uniform float exposureColor;\n"
        "void main() {\n"
        "    vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n"
        "    gl_FragColor = vec4(textureColor.r + exposureColor, textureColor.g + exposureColor, textureColor.b + exposureColor, textureColor.a);\n"
        "}\n";

#endif //TRINITY_GL_H
