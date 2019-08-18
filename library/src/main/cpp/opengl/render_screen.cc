//
// Created by wlanjie on 2019/4/13.
//

#include "render_screen.h"

void trinity::RenderScreen::OnDraw(GLuint texture_id) {
    glClearColor(0.0f, 1.0f, 0.0f, 0.5f);
    ProcessImage(texture_id);
}
