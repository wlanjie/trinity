/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by wlanjie on 2019/4/13.
//

#include "egl_core.h"
#include "android_xlog.h"

namespace trinity {

EGLCore::EGLCore() {
    pfneglPresentationTimeANDROID = 0;
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
}

EGLCore::~EGLCore() {
}

void EGLCore::Release() {
    if (EGL_NO_DISPLAY != display_ && EGL_NO_CONTEXT != context_) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        LOGI("after eglMakeCurrent...");
        eglDestroyContext(display_, context_);
        LOGI("after eglDestroyContext...");
        eglTerminate(display_);
        eglReleaseThread();
    }
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
}

void EGLCore::ReleaseSurface(EGLSurface eglSurface) {
    eglDestroySurface(display_, eglSurface);
    eglSurface = EGL_NO_SURFACE;
}

EGLContext EGLCore::GetContext() {
    LOGI("return EGLCore GetContext...");
    return context_;
}

EGLDisplay EGLCore::GetDisplay() {
    return display_;
}

EGLConfig EGLCore::GetConfig() {
    return config_;
}

EGLSurface EGLCore::CreateWindowSurface(ANativeWindow *_window) {
    EGLSurface surface = nullptr;
    EGLint format;

    if (_window == nullptr) {
        LOGE("EGLCore::CreateWindowSurface  window_ is NULL");
        return nullptr;
    }

    if (!eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &format)) {
        LOGE("eglGetConfigAttrib() returned error %d", eglGetError());
        Release();
        return surface;
    }
    ANativeWindow_setBuffersGeometry(_window, 0, 0, format);
    if (!(surface = eglCreateWindowSurface(display_, config_, _window, 0))) {
        LOGE("eglCreateWindowSurface() returned error %d", eglGetError());
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

int EGLCore::SetPresentationTime(EGLSurface surface, khronos_stime_nanoseconds_t nsecs) {
    pfneglPresentationTimeANDROID(display_, surface, nsecs);
    return 1;
}

int EGLCore::QuerySurface(EGLSurface surface, int what) {
    int value = -1;
    eglQuerySurface(display_, surface, what, &value);
    return value;
}

bool EGLCore::SwapBuffers(EGLSurface eglSurface) {
    return eglSwapBuffers(display_, eglSurface);
}

bool EGLCore::MakeCurrent(EGLSurface eglSurface) {
    return eglMakeCurrent(display_, eglSurface, eglSurface, context_);
}

void EGLCore::DoneCurrent() {
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool EGLCore::Init() {
    return this->Init(NULL);
}

bool EGLCore::Init(EGLContext sharedContext) {
    LOGI("enter: %s", __func__);
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
    context_ = eglCreateContext(display_, config_, NULL == sharedContext ? EGL_NO_CONTEXT : sharedContext, eglContextAttributes);
    if (context_ == NULL) {
        LOGE("eglCreateContext() returned error %d", eglGetError());
        Release();
        return false;
    }

    pfneglPresentationTimeANDROID = (PFNEGLPRESENTATIONTIMEANDROIDPROC)eglGetProcAddress("eglPresentationTimeANDROID");
    if (!pfneglPresentationTimeANDROID) {
        LOGE("eglPresentationTimeANDROID is not available!");
    }

    LOGI("leave: %s", __func__);
    return true;
}

}  // namespace trinity
