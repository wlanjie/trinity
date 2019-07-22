//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_RENDER_SCREEN_H
#define TRINITY_RENDER_SCREEN_H

#include "opengl.h"

namespace trinity {

class RenderScreen : public OpenGL {

public:
    void OnDraw(GLuint texture_id);
};

}

#endif //TRINITY_RENDER_SCREEN_H
