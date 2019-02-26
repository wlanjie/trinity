//
// Created by wlanjie on 2019/2/21.
//

#include "egl_core.h"
#include "egl_share_context.h"
#include "../../log.h"

namespace trinity {

EGLCore::EGLCore() {
    presentation_time_ = 0;
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
}

EGLCore::~EGLCore() {

}

bool EGLCore::Init() {
    return Init(nullptr);
}

bool EGLCore::Init(EGLContext shared_context) {
    EGLint numConfigs;
    EGLint width;
    EGLint height;

    const EGLint attribs[] = { EGL_BUFFER_SIZE, 32, EGL_ALPHA_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                               EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };

    if ((display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay() returned error %d", eglGetError());
        return false;
    }
    if (!eglInitialize(display_, 0, 0)) {
        LOGE("eglInitialize() returned error %d", eglGetError());
        return false;
    }

    if (!eglChooseConfig(display_, attribs, &config_, 1, &numConfigs)) {
        LOGE("eglChooseConfig() returned error %d", eglGetError());
        Release();
        return false;
    }

    EGLint eglContextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    if (!(context_ = eglCreateContext(display_, config_, NULL == shared_context ? EGL_NO_CONTEXT : shared_context, eglContextAttributes))) {
        LOGE("eglCreateContext() returned error %d", eglGetError());
        Release();
        return false;
    }

    presentation_time_ = (PFNEGLPRESENTATIONTIMEANDROIDPROC)eglGetProcAddress("eglPresentationTimeANDROID");
    if (!presentation_time_) {
        LOGE("eglPresentationTimeANDROID is not available!");
    }

    return true;
}

bool EGLCore::InitWithSharedContext() {
    EGLContext context = EGLShareContext::GetSharedContext();

    if (context == EGL_NO_CONTEXT){
        return false;
    }

    return Init(context);
}

EGLSurface EGLCore::CreateWindowSurface(ANativeWindow *window) {
    EGLSurface surface = nullptr;
    EGLint format;

    if (window == nullptr) {
        LOGI("EGLCore::CreateWindowSurface window is nullptr");
        return nullptr;
    }
    if (!eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &format)) {
        LOGE("eglGetConfigAttrib() returned error %d", eglGetError());
        Release();
        return surface;
    }
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);
    if (!(surface = eglCreateWindowSurface(display_, config_, window, 0))) {
        LOGE("eglCreateWindowSurface() return error %d", eglGetError());
    }
    return surface;
}

EGLSurface EGLCore::CreateOffscreenSurface(int width, int height) {
    EGLSurface surface;
    EGLint PbufferAttributes[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE, EGL_NONE };
    if (!(surface = eglCreatePbufferSurface(display_, config_, PbufferAttributes))) {
        LOGE("eglCreatePbufferSurface() returned error %d", eglGetError());
    }
    return surface;
}

bool EGLCore::MakeCurrent(EGLSurface surface) {
    return eglMakeCurrent(display_, surface, surface, context_);
}

void EGLCore::DoneCurrent() {
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool EGLCore::SwapBuffers(EGLSurface surface) {
    return eglSwapBuffers(display_, surface);
}

int EGLCore::QuerySurface(EGLSurface surface, int what) {
    int value = -1;
    eglQuerySurface(display_, surface, what, &value);
    return value;
}

int EGLCore::SetPresentationTime(EGLSurface surface, khronos_stime_nanoseconds_t nsecs) {
    presentation_time_(display_, surface, nsecs);
    return 1;
}

void EGLCore::ReleaseSurface(EGLSurface surface) {
    if (EGL_NO_DISPLAY != display_) {
        eglDestroySurface(display_, surface);
        surface = EGL_NO_SURFACE;
    }
}

void EGLCore::Release() {
    if (EGL_NO_DISPLAY != display_ && EGL_NO_CONTEXT != context_) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display_, context_);
    }
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
}

EGLContext EGLCore::GetContext() {
    return context_;
}

EGLDisplay EGLCore::GetDisplay() {
    return display_;
}

EGLConfig EGLCore::GetConfig() {
    return config_;
}
}