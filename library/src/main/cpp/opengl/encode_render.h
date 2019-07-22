//
// Created by wlanjie on 2019/4/26.
//

#ifndef TRINITY_ENCODE_RENDER_H
#define TRINITY_ENCODE_RENDER_H

#include "opengl.h"

namespace trinity {

class EncodeRender : public OpenGL {
public:
    EncodeRender();
    EncodeRender(int width, int height);
    virtual ~EncodeRender();

    void CopyYUV2Image(GLuint texture, uint8_t* buffer, int width, int height);
    void Destroy();

protected:
    virtual void RunOnDrawTasks();
    virtual void OnDrawArrays();

private:
    void ConvertYUY2(int texture_id, int width, int height);
    void DownloadImageFromTexture(int texture_id, void* buffer, int width, int height);
    int GetBufferSizeInBytes(int width, int height);
    void EnsureTextureStorage(GLint internal_format, int yuy2_pairs, int height);
private:
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    bool recording_;
    GLuint fbo_;
    GLuint download_texture_;
};

}

#endif //TRINITY_ENCODE_RENDER_H
