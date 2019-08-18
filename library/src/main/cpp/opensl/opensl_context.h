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
        return slCreateEngine(&engine_object_, ARRAY_LEN(engineOptions), engineOptions, 0, // no interfaces
                              0, // no interfaces
                              0); // no required
    };

    /**
     * Realize the given object. Objects needs to be
     * realized before using them.
     * @param object object instance.
     */
    SLresult RealizeObject(SLObjectItf object) {
        // Realize the engine object
        return (*object)->Realize(object, SL_BOOLEAN_FALSE); // No async, blocking call
    };

    /**
     * Gets the engine interface from the given engine object
     * in order to create other objects from the engine.
     */
    SLresult GetEngineInterface() {
        // Get the engine interface
        return (*engine_object_)->GetInterface(engine_object_, SL_IID_ENGINE, &engine_engine_);
    };

    void Init();

    static OpenSLContext *instance_;
    OpenSLContext();
public:
    static OpenSLContext *GetInstance(); //工厂方法(用来获得实例)
    virtual ~OpenSLContext();

    SLEngineItf GetEngine() {
        return engine_engine_;
    };
};

}

#endif //TRINITY_OPENSL_CONTEXT_H
