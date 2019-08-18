//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_EGL_CORE_H
#define TRINITY_EGL_CORE_H

#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <KHR/khrplatform.h>

typedef EGLBoolean (EGLAPIENTRYP PFNEGLPRESENTATIONTIMEANDROIDPROC)(EGLDisplay display, EGLSurface surface, khronos_stime_nanoseconds_t time);

namespace trinity {

class EGLCore {
public:
    EGLCore();
    virtual ~EGLCore();
    bool Init();
    bool Init(EGLContext sharedContext);
    bool InitWithSharedContext();
    EGLSurface CreateWindowSurface(ANativeWindow *_window);
    EGLSurface CreateOffscreenSurface(int width, int height);
    bool MakeCurrent(EGLSurface eglSurface);
    void DoneCurrent();
    bool SwapBuffers(EGLSurface eglSurface);
    int QuerySurface(EGLSurface surface, int what);
    int SetPresentationTime(EGLSurface surface, khronos_stime_nanoseconds_t nsecs);
    void ReleaseSurface(EGLSurface eglSurface);
    void Release();
    EGLContext GetContext();
    EGLDisplay GetDisplay();
    EGLConfig GetConfig();

private:
    EGLDisplay display_;
    EGLConfig config_;
    EGLContext context_;

    PFNEGLPRESENTATIONTIMEANDROIDPROC pfneglPresentationTimeANDROID;
};

}

#endif //TRINITY_EGL_CORE_H
