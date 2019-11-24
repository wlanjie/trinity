//
//  mat4.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//
#include "video_render_types.h"

#ifndef mat4_h
#define mat4_h

void identity(GLfloat * out);
void perspective (GLfloat fovy, GLfloat aspect, GLfloat near, GLfloat far, GLfloat * out);
void lookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
            GLfloat centerx, GLfloat centery, GLfloat centerz,
            GLfloat upx, GLfloat upy, GLfloat upz,
            GLfloat * out);
__attribute__((unused))
void flipX(GLfloat * a);
void rotateX(GLfloat * out, GLfloat rad);
void rotateY(GLfloat * out, GLfloat rad);
void rotateZ(GLfloat * out, GLfloat rad);
void multiply(GLfloat * out, GLfloat * a, GLfloat * b);

__attribute__((unused))
void setRotateEulerM(GLfloat * out, GLfloat x, GLfloat y, GLfloat z);
__attribute__((unused))
void clone(GLfloat * out, GLfloat * a);
__attribute__((unused))
void transformVec4(GLfloat * out, GLfloat * a, GLfloat * m);

#endif // mat4_h
