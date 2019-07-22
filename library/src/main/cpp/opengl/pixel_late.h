//
// Created by wlanjie on 2019-05-26.
//

#ifndef TRINITY_PIXEL_LATE_H
#define TRINITY_PIXEL_LATE_H

#include "opengl.h"
#include "frame_buffer.h"

namespace trinity {

class PixelLate : public FrameBuffer {

public:
    PixelLate(int width, int height);
    ~PixelLate();
    GLuint OnDrawFrame(int textureId);

protected:
    virtual void RunOnDrawTasks();
    virtual void OnDrawArrays();

private:
    int width_;
    int height_;

};

}

#endif //TRINITY_PIXEL_LATE_H
