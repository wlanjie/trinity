//
// Created by wlanjie on 2019/4/13.
//

#include "egl_share_context.h"
#include "android_xlog.h"

namespace trinity {

EGLShareContext* EGLShareContext::instance_ = new EGLShareContext();

EGLShareContext::EGLShareContext() {
    Init();
}

void EGLShareContext::Init() {
    LOGI("Create EGLContext EGLCore::Init");
    shared_context_ = EGL_NO_CONTEXT;
    EGLint numConfigs;
    shared_display_ = EGL_NO_DISPLAY;
    EGLConfig config;
    const EGLint attribs[] = { EGL_BUFFER_SIZE, 32, EGL_ALPHA_SIZE, 8,
                               EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                               EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE,
                               EGL_WINDOW_BIT, EGL_NONE };
    if ((shared_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY))
        == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay() returned error %d", eglGetError());
        return;
    }
    if (!eglInitialize(shared_display_, 0, 0)) {
        LOGE("eglInitialize() returned error %d", eglGetError());
        return;
    }

    if (!eglChooseConfig(shared_display_, attribs, &config, 1, &numConfigs)) {
        LOGE("eglChooseConfig() returned error %d", eglGetError());
        return;
    }

    EGLint eglContextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    if (!(shared_context_ = eglCreateContext(shared_display_, config,
                                             NULL == shared_context_ ? EGL_NO_CONTEXT : shared_context_,
                                             eglContextAttributes))) {
        LOGE("eglCreateContext() returned error %d", eglGetError());
        return;
    }
}



}