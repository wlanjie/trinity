//
// Created by wlanjie on 2019-12-19.
//

#ifndef TRINITY_PROCESS_BUFFER_H
#define TRINITY_PROCESS_BUFFER_H

#if __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#endif

namespace trinity {

class ProcessBuffer {
 public:
    ProcessBuffer();
    ~ProcessBuffer();

    int Init(const char* vertex_shader, const char* fragment_shader);
    void SetOutput(int width, int height);
    void SetInt(const char* name, int value);
    void SetFloat(const char* name, float value);
    void SetFloatVec2(const char* name, int size, const GLfloat* value);
    void SetFloatVec3(const char* name, int size, const GLfloat* value);
    void SetFloatVec4(const char* name, int size, const GLfloat* value);
    void SetUniformMatrix3f(const char* name, int size, const GLfloat* matrix);
    void SetUniformMatrix4f(const char* name, int size, const GLfloat* matrix);
    void Destroy();

    void ActiveProgram();
    void Clear();
    void ActiveAttribute();
    void DrawArrays();
    GLuint GetFrameBufferId() {
        return frame_buffer_id_;
    }
    GLuint GetTextureId() {
        return texture_id_;
    }

 private:
    void CreateProgram(const char *vertex, const char *fragment);
    void CompileShader(const char *shader_string, GLuint shader);
    void Link();

 private:
    GLuint program_;
    int width_;
    int height_;
    GLuint frame_buffer_id_;
    GLuint texture_id_;
    GLfloat* default_vertex_coordinates_;
    GLfloat* default_texture_coordinates_;
};

}
#endif //TRINITY_PROCESS_BUFFER_H
