#include "egl_share_context.h"

#define LOG_TAG "EglShareContext"
EglShareContext* EglShareContext::instance_ = new EglShareContext();

EglShareContext::EglShareContext() {
	init();
}

void EglShareContext::init() {
	LOGI("Create EGLContext EGLCore::Init");
	sharedContext = EGL_NO_CONTEXT;
	EGLint numConfigs;
	sharedDisplay = EGL_NO_DISPLAY;
	EGLConfig config;
	const EGLint attribs[] = { EGL_BUFFER_SIZE, 32, EGL_ALPHA_SIZE, 8,
			EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE,
			EGL_WINDOW_BIT, EGL_NONE };
	if ((sharedDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY))
			== EGL_NO_DISPLAY) {
		LOGE("eglGetDisplay() returned error %d", eglGetError());
		return;
	}
	if (!eglInitialize(sharedDisplay, 0, 0)) {
		LOGE("eglInitialize() returned error %d", eglGetError());
		return;
	}

	if (!eglChooseConfig(sharedDisplay, attribs, &config, 1, &numConfigs)) {
		LOGE("eglChooseConfig() returned error %d", eglGetError());
		return;
	}

	EGLint eglContextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	if (!(sharedContext = eglCreateContext(sharedDisplay, config,
			NULL == sharedContext ? EGL_NO_CONTEXT : sharedContext,
			eglContextAttributes))) {
		LOGE("eglCreateContext() returned error %d", eglGetError());
		return;
	}
}

