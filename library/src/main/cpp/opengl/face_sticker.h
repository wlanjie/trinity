//
// Created by wlanjie on 2020-02-01.
//

#ifndef TRINITY_FACE_STICKER_H
#define TRINITY_FACE_STICKER_H

#include "opengl.h"

namespace trinity {

class FaceSticker : public OpenGL {
 public:
    FaceSticker();
    ~FaceSticker();
    int OnDrawFrame(int texture_id);

 protected:
    virtual void RunOnDrawTasks();
 private:
    void CreateStickerTexture();
    void SetupLandMarks();

 private:
    GLuint sticker_texture_id_;
};

}

#endif  // TRINITY_FACE_STICKER_H
