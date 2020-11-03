//
//  jni_reflect.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef jni_reflect_h
#define jni_reflect_h
#include "play_types.h"

void jni_reflect_java_class(JavaClass ** p_jc, JNIEnv *env);
void jni_free(JavaClass **p_jc, JNIEnv *env);
#endif  // jni_reflect_h
