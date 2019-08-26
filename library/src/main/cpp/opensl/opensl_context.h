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
 *
 */

//
// Created by wlanjie on 2019/4/21.
//

#ifndef TRINITY_OPENSL_CONTEXT_H
#define TRINITY_OPENSL_CONTEXT_H

#include "opensl_util.h"
#include "tools.h"

namespace trinity {

class OpenSLContext {
 private:
    SLObjectItf engine_object_;
    SLEngineItf engine_engine_;
    bool inited_;

    /**
     * Creates an OpenSL ES engine.
     */
    SLresult CreateEngine() {
        // OpenSL ES for Android is designed to be thread-safe,
        // so this option request will be ignored, but it will
        // make the source code portable to other platforms.
        SLEngineOption engineOptions[] = {{(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE}};

        // Create the OpenSL ES engine object
        return slCreateEngine(&engine_object_, ARRAY_LEN(engineOptions), engineOptions, 0,
                              0,
                              0);
    }

    /**
     * Realize the given object. Objects needs to be
     * realized before using them.
     * @param object object instance.
     */
    SLresult RealizeObject(SLObjectItf object) {
        // Realize the engine object
        return (*object)->Realize(object, SL_BOOLEAN_FALSE);
    }

    /**
     * Gets the engine interface from the given engine object
     * in order to create other objects from the engine.
     */
    SLresult GetEngineInterface() {
        // Get the engine interface
        return (*engine_object_)->GetInterface(engine_object_, SL_IID_ENGINE, &engine_engine_);
    }

    void Init();

    static OpenSLContext *instance_;
    OpenSLContext();

 public:
    static OpenSLContext *GetInstance();
    virtual ~OpenSLContext();

    SLEngineItf GetEngine() {
        return engine_engine_;
    }
};

}  // namespace trinity

#endif  // TRINITY_OPENSL_CONTEXT_H
