#ifndef ANDROID_EGL_CORE_H_
#define ANDROID_EGL_CORE_H_

#include "common_tools.h"
#include <pthread.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <KHR/khrplatform.h>

typedef EGLBoolean (EGLAPIENTRYP PFNEGLPRESENTATIONTIMEANDROIDPROC)(EGLDisplay display, EGLSurface surface, khronos_stime_nanoseconds_t time);

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
#endif // ANDROID_EGL_CORE_H_
