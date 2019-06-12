//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_OPENGL_H
#define TRINITY_OPENGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace trinity {

enum TextureType {
    TEXTURE_OES = 0,
    TEXTURE_2D
};

class OpenGL {
public:
    OpenGL();
    OpenGL(int width, int height);
    OpenGL(const char* vertex, const char* fragment);
    OpenGL(int width, int height, const char* vertex, const char* fragment);
    ~OpenGL();
    void SetTextureType(TextureType type = TEXTURE_2D);
    void Init(const char* vertex, const char* fragment);
    void SetOutput(int width, int height);
    void SetInt(const char* name, int value);
    void SetFloat(const char* name, float value);
    void SetFloatVec2(const char* name, int size, const GLfloat* value);
    void SetFloatVec3(const char* name, int size, const GLfloat* value);
    void SetFloatVec4(const char* name, int size, const GLfloat* value);
    void SetUniformMatrix3f(const char* name, int size, const GLfloat* matrix);
    void SetUniformMatrix4f(const char* name, int size, const GLfloat* matrix);

    void ProcessImage(GLuint texture_id);
    void ProcessImage(GLuint texture_id, GLfloat* texture_matrix);
    void ProcessImage(GLuint texture_id, const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate);
    void ProcessImage(GLuint texture_id, const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate, GLfloat* texture_matrix);

protected:
    virtual void RunOnDrawTasks();
    virtual void OnDrawArrays();

private:
    void CreateProgram(const char* vertex, const char* fragment);
    void CompileShader(const char* shader_string, GLuint shader);
    void Link();

private:
    TextureType type_;
    GLuint program_;
    int width_;
    int height_;
};

}

#endif //TRINITY_OPENGL_H
