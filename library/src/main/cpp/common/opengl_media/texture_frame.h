//
// Created by wlanjie on 2019/2/22.
//

#ifndef TRINITY_TEXTURE_FRAME_H
#define TRINITY_TEXTURE_FRAME_H

#include <GLES2/gl2.h>

namespace trinity {

class TextureFrame {
public:
    TextureFrame();
    virtual ~TextureFrame();

    virtual bool CreateTexture() = 0;
    virtual void UpdateTextureImage() = 0;
    virtual bool BindTexture(GLint* uniform_samplers) = 0;
    virtual void Dealloc() = 0;

protected:
    bool checkGlError(const char* op);
};

}

#endif //TRINITY_TEXTURE_FRAME_H
