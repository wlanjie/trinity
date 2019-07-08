//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_FRAME_BUFFER_H
#define TRINITY_FRAME_BUFFER_H

#include "opengl.h"

#define FRAME_VERTICAL                                            0
#define FRAME_HORIZONTAL                                          1
#define FRAME_SQUARE                                              2

namespace trinity {

class FrameBuffer : public OpenGL {
public:
    FrameBuffer(int width, int height, const char* vertex, const char* fragment);
    FrameBuffer(int width, int height, int view_width, int view_height, const char* vertex, const char* fragment);
    void CompileFrameBuffer(int width, int height, const char* vertex, const char* fragment);
    ~FrameBuffer();

//    void Init(const char* vertex, const char* fragment);
    GLuint OnDrawFrame(GLuint texture_id);
    GLuint OnDrawFrame(GLuint texture_id, GLfloat* matrix);
    GLuint OnDrawFrame(GLuint texture_id, const GLfloat* vertex_coordinate, const GLfloat* texture_coordinate);

    GLuint GetTextureId() {
        return texture_id_;
    }

    void SetFrame(int frame, int screen_width, int screen_height);

private:
    void ResetVertexCoordinate();
private:
    GLuint texture_id_;
    GLuint frameBuffer_id_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
};

}

#endif //TRINITY_FRAME_BUFFER_H
