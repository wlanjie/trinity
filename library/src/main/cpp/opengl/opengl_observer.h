//
//  opengl_observer.h
//  opengl
//
//  Created by wlanjie on 2019/8/30.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef opengl_observer_h
#define opengl_observer_h

namespace trinity {

class OnGLObserver {
 public:
    virtual void OnGLParams() = 0;
    virtual void OnGLArrays() = 0;
};    
    
}

#endif /* opengl_observer_h */
