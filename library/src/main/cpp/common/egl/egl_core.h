//
// Created by wlanjie on 2019/2/21.
//

#ifndef TRINITY_EGL_CORE_H
#define TRINITY_EGL_CORE_H

#include <pthread.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <KHR/khrplatform.h>

namespace trinity {

class EGLCore {
public:
    EGLCore();
    virtual ~EGLCore();

    bool Init();

    bool Init(EGLContext shared_context);

    bool InitWithSharedContext();

    EGLSurface CreateWindowSurface(ANativeWindow* window);

    EGLSurface CreateOffscreenSurface(int width, int height);

    bool MakeCurrent(EGLSurface surface);

    void DoneCurrent();

    bool SwapBuffers(EGLSurface surface);

    int QuerySurface(EGLSurface surface, int what);

    int SetPresentationTime(EGLSurface surface, khronos_stime_nanoseconds_t nsecs);

    void ReleaseSurface(EGLSurface surface);

    void Release();

    EGLContext GetContext();

    EGLDisplay GetDisplay();

    EGLConfig GetConfig();

private:
    EGLDisplay display_;
    EGLConfig config_;
    EGLContext context_;

    PFNEGLPRESENTATIONTIMEANDROIDPROC presentation_time_;
};

}
#endif //TRINITY_EGL_CORE_H
