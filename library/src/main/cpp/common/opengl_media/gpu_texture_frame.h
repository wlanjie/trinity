//
// Created by wlanjie on 2019/2/22.
//

#ifndef TRINITY_GPU_TEXTURE_FRAME_H
#define TRINITY_GPU_TEXTURE_FRAME_H

#include "texture_frame.h"

namespace trinity {

class GPUTextureFrame : public TextureFrame {
public:
    GPUTextureFrame();
    ~GPUTextureFrame();

    bool CreateTexture();
    void UpdateTextureImage();
    bool BindTexture(GLint* uniform_samplers);
    void Dealloc();

    GLuint GetDecodeTextureId() {
        return decode_texture_id_;
    }

private:
    GLuint decode_texture_id_;
    int InitTexture();
};

}
#endif //TRINITY_GPU_TEXTURE_FRAME_H
