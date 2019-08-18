//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_EGL_SHARE_CONTEXT_H
#define TRINITY_EGL_SHARE_CONTEXT_H

#include <EGL/egl.h>

namespace trinity {

class EGLShareContext {
public:
    static EGLContext getSharedContext() {
        if (instance_->shared_display_ == EGL_NO_DISPLAY){
            instance_->Init();
        }
        return instance_->shared_context_;
    }

private:
    EGLShareContext();
    void Init();
private:
    static EGLShareContext* instance_;
    EGLContext shared_context_;
    EGLDisplay shared_display_;
};

}

#endif //TRINITY_EGL_SHARE_CONTEXT_H
