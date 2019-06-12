//
// Created by wlanjie on 2019/4/21.
//

#include "opensl_context.h"

namespace trinity {

OpenSLContext* OpenSLContext::instance_ = new trinity::OpenSLContext();

void OpenSLContext::Init() {
    SLresult result = CreateEngine();
    if (SL_RESULT_SUCCESS == result) {
        // Realize the engine object
        result = RealizeObject(engine_object_);
        if (SL_RESULT_SUCCESS == result) {
            // Get the engine interface
            result = GetEngineInterface();
        }
    }
}

OpenSLContext::OpenSLContext() {
    inited_ = false;
}
OpenSLContext::~OpenSLContext() {
}

OpenSLContext* OpenSLContext::GetInstance() {
    if (!instance_->inited_) {
        instance_->Init();
        instance_->inited_ = true;
    }
    return instance_;
}



}