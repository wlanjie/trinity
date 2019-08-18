//
// Created by wlanjie on 2019-08-17.
//

#ifndef TRINITY_RENDER_CALLBACK_H
#define TRINITY_RENDER_CALLBACK_H

#include "message_queue.h"

namespace trinity {

class GLObserver {

public:
    virtual void OnGLCreate() = 0;
    virtual void OnGLMessage(Message* msg) = 0;
    virtual void OnGLDestroy() = 0;
};

}

#endif //TRINITY_RENDER_CALLBACK_H
