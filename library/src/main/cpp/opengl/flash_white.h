//
// Created by wlanjie on 2019-06-03.
//

#ifndef TRINITY_FLASH_WHITE_H
#define TRINITY_FLASH_WHITE_H

#include "frame_buffer.h"
#include "gl.h"

#define MAX_FRAMES 16
#define SKIP_FRAMES 4

namespace trinity {

class FlashWhite : public FrameBuffer {
public:
    FlashWhite(int width, int height) : FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, FLASH_WHITE_FRAGMENT_SHADER) {
        progress = 0;
        frames = 0;
        SetOutput(width, height);
    }

protected:
    virtual void RunOnDrawTasks() {
        FrameBuffer::RunOnDrawTasks();
        if (frames <= MAX_FRAMES) {
            progress = frames * 1.0f / MAX_FRAMES;
        } else {
            progress = 2.0f - frames * 1.0f / MAX_FRAMES;
        }
        progress = (float) frames / MAX_FRAMES;
        if (progress > 1) {
            progress = 0;
        }
        frames++;
        if (frames > MAX_FRAMES) {
            frames = 0;
        }
        SetFloat("exposureColor", progress);
    }

private:
    int frames;
    float progress;
};

}

#endif //TRINITY_FLASH_WHITE_H
