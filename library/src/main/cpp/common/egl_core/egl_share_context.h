#ifndef ANDROID_EGL_SHARE_CONTEXT_H_
#define ANDROID_EGL_SHARE_CONTEXT_H_

#include "./egl_core.h"

class EglShareContext {
public:
	~EglShareContext() {
	}
	static EGLContext getSharedContext() {
		if (instance_->shared_display_ == EGL_NO_DISPLAY){
			instance_->Init();
		}
		return instance_->shared_context_;
	}
protected:
	EglShareContext();
	void Init();


private:
	static EglShareContext* instance_;
	EGLContext shared_context_;
	EGLDisplay shared_display_;
};

#endif
