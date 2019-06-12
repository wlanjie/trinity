//
// Created by wlanjie on 2019/4/10.
//

#ifndef TRINITY_YUV_RENDER_H
#define TRINITY_YUV_RENDER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include "egl_core.h"
#include "ffplay.h"

#define VERTEX_COORDINATE_COUNT			8

namespace trinity {

class YuvRender {
public:
    YuvRender(int width, int height, int degree);

    ~YuvRender();

    virtual int DrawFrame(AVFrame *frame);

private:
    int Initialize(int width, int height);
    void Destroy();
    void CopyFrame(uint8_t *dst, uint8_t *src, int width, int height, int line_size);
    void CreateProgram(const char* vertex, const char* fragment);
    void CompileShader(const char* shader_string, GLuint shader);
    void Link();

private:
    GLuint frame_buffer_id_;
    GLuint texture_id_;
    GLfloat *vertex_coordinate_;
    GLfloat *texture_coordinate_;
    GLuint textures[3];
    GLuint program_;
    GLint vertex_coordinate_location_;
    GLint texture_coordinate_location_;
    GLint uniform_samplers_[3];

    uint8_t *y;
    uint8_t *u;
    uint8_t *v;
    int y_size_;
    int u_size_;
    int v_size_;
};

}
#endif //TRINITY_YUV_RENDER_H
