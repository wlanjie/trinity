//
// Created by wlanjie on 2019-12-26.
//

#ifndef TRINITY_BLEND_H
#define TRINITY_BLEND_H

#include "opengl.h"
#include "gl.h"

static const char* BLEND_VERTEX_SHADER = 
        "attribute vec3 position;                                                               \n"        
        "attribute vec2 inputTextureCoordinate;                                                 \n"
        "varying vec2 textureCoordinate;                                                        \n"
        "varying vec2 textureCoordinate2;                                                       \n"
        "uniform mat4 matrix;                                                                   \n"
        "void main() {                                                                          \n"
        "    vec4 p = vec4(position, 1.);                                                       \n"
        "    textureCoordinate = inputTextureCoordinate;                                        \n"
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

namespace trinity {

class Blend {
 public:
    Blend();
    ~Blend();
    
    int OnDrawFrame(int texture_id, int sticker_texture_id, GLfloat* matrix, float alpha_factor);
 private:
    int CreateProgram(const char* vertex, const char* fragment);
    void CompileShader(const char* shader_string, GLuint shader);
    int Link(int program);

 private:
    int program_;
    GLint input_image_texture2_;
    GLint matrix_location_;
    int second_program_;
    GLuint frame_buffer_id_;
    GLuint frame_buffer_texture_id_;
    GLfloat* default_vertex_coordinates_;
    GLfloat* default_texture_coordinates_;
};

}

#endif  // TRINITY_BLEND_H
