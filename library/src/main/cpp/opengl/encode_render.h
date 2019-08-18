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
        virtual ~EncodeRender();

        void CopyYUV420Image(GLuint texture, uint8_t *buffer, int width, int height);
        void Destroy();

    protected:
        virtual void RunOnDrawTasks();

    private:
        void ConvertYUV420(int texture_id, int width, int height, void *buffer);
    private:
        int width_;
        int height_;
    };

}

#endif //TRINITY_ENCODE_RENDER_H
