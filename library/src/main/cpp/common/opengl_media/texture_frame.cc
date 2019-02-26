//
// Created by wlanjie on 2019/2/22.
//

#include "texture_frame.h"
#include "../../log.h"

namespace trinity {

TextureFrame::TextureFrame() {

}

TextureFrame::~TextureFrame() {

}

bool TextureFrame::checkGlError(const char *op) {
    GLint error;
    for (error = glGetError(); error; error = glGetError()) {
        LOGE("error: after %s() glError (0x%x)", op, error);
        return true;
    }
    return false;
}

}
