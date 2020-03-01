//
// Created by wlanjie on 2020-02-01.
//

#include "face_sticker.h"

static const char* FACE_STICKER_VERTEX_SHADER =
        "precision highp float;"
        "uniform mat4 mvpMatrix;"
        "attribute vec4 position;"
        "attribute vec4 inputTextureCoordinate;"
        "varying vec2 textureCoordinate;"
        "void main() {"
        "   gl_Position = mvpMatrix * position;"
        "   textureCoordinate = inputTextureCoordinate.xy;"
        "}";

static const char* FACE_STICKER_FRAGMENT_SHADER =
        "precision highp float;"
        "varying vec2 textureCoordinate;"
        "uniform sampler2D inputImageTexture2;"
        "uniform int enableSticker;"
        "vec4 blendColor(vec4 frameColor, vec4 sourceColor) {"
        "   vec4 outputColor;"
        "   outputColor.r = frameColor.r * frameColor.a + sourceColor.r * sourceColor.a * (1.0 - frameColor.a);"
        "   outputColor.g = frameColor.g * frameColor.a + sourceColor.g * sourceColor.a * (1.0 - frameColor.a);"
        "   outputColor.b = frameColor.b * frameColor.a + sourceColor.b * sourceColor.a * (1.0 - frameColor.a);"
        "   outputColor.a = frameColor.a + sourceColor.a * (1.0 - frameColor.a);"
        "   return outputColor;"
        "}"
        "void main() {"
        "   lowp vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);"
        "   if (enableSticker == 0) {"
        "       gl_FragColor = sourceColor;"
        "   } else {"
        "       lowp vec4 frameColor = texture2D(inputImageTexture2, textureCoordinate);"
        "       gl_FragColor = blendColor(frameColor, sourceColor);"
        "   }"
        "}";

namespace trinity {

FaceSticker::FaceSticker() : OpenGL(FACE_STICKER_VERTEX_SHADER, FACE_STICKER_FRAGMENT_SHADER) {
    CreateStickerTexture();
}

FaceSticker::~FaceSticker() {

}

int FaceSticker::OnDrawFrame(int texture_id) {
    return 0;
}

void FaceSticker::RunOnDrawTasks() {

}

void FaceSticker::CreateStickerTexture() {
    glGenTextures(1, &sticker_texture_id_);
    glBindTexture(GL_TEXTURE_2D, sticker_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void FaceSticker::SetupLandMarks() {

}

}