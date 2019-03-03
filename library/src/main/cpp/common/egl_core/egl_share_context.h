#ifndef ANDROID_EGL_SHARE_CONTEXT_H_
#define ANDROID_EGL_SHARE_CONTEXT_H_

#include "./egl_core.h"

class EglShareContext {
public:
	~EglShareContext() {
	}
	static EGLContext getSharedContext() {
		if (instance_->sharedDisplay == EGL_NO_DISPLAY){
			instance_->init();
		}
		return instance_->sharedContext;
	}
protected:
	EglShareContext();
	void init();


private:
	static EglShareContext* instance_;
	EGLContext sharedContext;
	EGLDisplay sharedDisplay;
};

#endif
