//
//  mat4.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "mat4.h"
#define PI 3.141592653589793
void identity(GLfloat * out){
    out[0] = 1;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;

    out[4] = 0;
    out[5] = 1;
    out[6] = 0;
    out[7] = 0;

    out[8] = 0;
    out[9] = 0;
    out[10] = 1;
    out[11] = 0;

    out[12] = 0;
    out[13] = 0;
    out[14] = 0;
    out[15] = 1;
}

void perspective (GLfloat fovy, GLfloat aspect, GLfloat near, GLfloat far, GLfloat * out){
    GLfloat f = (GLfloat) (1.0 / tanf((float) (fovy * PI / 360))), nf = 1 / (near - far);
    out[0] = f / aspect;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;
    out[4] = 0;
    out[5] = f;
    out[6] = 0;
    out[7] = 0;
    out[8] = 0;
    out[9] = 0;
    out[10] = (far + near) * nf;
    out[11] = -1;
    out[12] = 0;
    out[13] = 0;
    out[14] = (2 * far * near) * nf;
    out[15] = 0;
}

void lookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
            GLfloat centerx, GLfloat centery, GLfloat centerz,
            GLfloat upx, GLfloat upy, GLfloat upz,
            GLfloat * out){
    GLfloat x0, x1, x2, y0, y1, y2, z0, z1, z2, len;
    if (fabsf(eyex - centerx) < 0.000001 &&
            fabsf(eyey - centery) < 0.000001 &&
            fabsf(eyez - centerz) < 0.000001) {
        identity(out);
        return;
    }

    z0 = eyex - centerx;
    z1 = eyey - centery;
    z2 = eyez - centerz;

    len = 1 / sqrtf(z0 * z0 + z1 * z1 + z2 * z2);
    z0 *= len;
    z1 *= len;
    z2 *= len;

    x0 = upy * z2 - upz * z1;
    x1 = upz * z0 - upx * z2;
    x2 = upx * z1 - upy * z0;
    len = sqrtf(x0 * x0 + x1 * x1 + x2 * x2);
    if (!len) {
        x0 = 0;
        x1 = 0;
        x2 = 0;
    } else {
        len = 1 / len;
        x0 *= len;
        x1 *= len;
        x2 *= len;
    }

    y0 = z1 * x2 - z2 * x1;
    y1 = z2 * x0 - z0 * x2;
    y2 = z0 * x1 - z1 * x0;

    len = sqrtf(y0 * y0 + y1 * y1 + y2 * y2);
    if (!len) {
        y0 = 0;
        y1 = 0;
        y2 = 0;
    } else {
        len = 1 / len;
        y0 *= len;
        y1 *= len;
        y2 *= len;
    }

    out[0] = x0;
    out[1] = y0;
    out[2] = z0;
    out[3] = 0;
    out[4] = x1;
    out[5] = y1;
    out[6] = z1;
    out[7] = 0;
    out[8] = x2;
    out[9] = y2;
    out[10] = z2;
    out[11] = 0;
    out[12] = -(x0 * eyex + x1 * eyey + x2 * eyez);
    out[13] = -(y0 * eyex + y1 * eyey + y2 * eyez);
    out[14] = -(z0 * eyex + z1 * eyey + z2 * eyez);
    out[15] = 1;
}

__attribute__((unused))
void flipX(GLfloat * a){
    GLfloat flipMat[16] = {
            -1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
    };
    multiply(a, flipMat, a);
}

void rotateX(GLfloat * out, GLfloat rad){
    GLfloat s = sinf(rad),
            c = cosf(rad),
            a10 = out[4],
            a11 = out[5],
            a12 = out[6],
            a13 = out[7],
            a20 = out[8],
            a21 = out[9],
            a22 = out[10],
            a23 = out[11];
    out[4] = a10 * c + a20 * s;
    out[5] = a11 * c + a21 * s;
    out[6] = a12 * c + a22 * s;
    out[7] = a13 * c + a23 * s;
    out[8] = a20 * c - a10 * s;
    out[9] = a21 * c - a11 * s;
    out[10] = a22 * c - a12 * s;
    out[11] = a23 * c - a13 * s;
}

void rotateY(GLfloat * out, GLfloat rad){
    GLfloat s = sinf(rad),
            c = cosf(rad),
            a00 = out[0],
            a01 = out[1],
            a02 = out[2],
            a03 = out[3],
            a20 = out[8],
            a21 = out[9],
            a22 = out[10],
            a23 = out[11];
    out[0] = a00 * c - a20 * s;
    out[1] = a01 * c - a21 * s;
    out[2] = a02 * c - a22 * s;
    out[3] = a03 * c - a23 * s;
    out[8] = a00 * s + a20 * c;
    out[9] = a01 * s + a21 * c;
    out[10] = a02 * s + a22 * c;
    out[11] = a03 * s + a23 * c;
}

void rotateZ(GLfloat * out, GLfloat rad){
    GLfloat s = sinf(rad),
            c = cosf(rad),
            a00 = out[0],
            a01 = out[1],
            a02 = out[2],
            a03 = out[3],
            a10 = out[4],
            a11 = out[5],
            a12 = out[6],
            a13 = out[7];
    out[0] = a00 * c + a10 * s;
    out[1] = a01 * c + a11 * s;
    out[2] = a02 * c + a12 * s;
    out[3] = a03 * c + a13 * s;
    out[4] = a10 * c - a00 * s;
    out[5] = a11 * c - a01 * s;
    out[6] = a12 * c - a02 * s;
    out[7] = a13 * c - a03 * s;
}
void multiply(GLfloat * out, GLfloat * a, GLfloat * b){
    GLfloat a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3],
            a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7],
            a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11],
            a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

    GLfloat b0  = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
    out[0] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    out[1] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    out[2] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    out[3] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

    b0 = b[4]; b1 = b[5]; b2 = b[6]; b3 = b[7];
    out[4] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    out[5] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    out[6] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    out[7] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

    b0 = b[8]; b1 = b[9]; b2 = b[10]; b3 = b[11];
    out[8] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    out[9] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    out[10] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    out[11] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

    b0 = b[12]; b1 = b[13]; b2 = b[14]; b3 = b[15];
    out[12] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    out[13] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    out[14] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    out[15] = b0*a03 + b1*a13 + b2*a23 + b3*a33;
}

__attribute__((unused))
void setRotateEulerM(GLfloat * out, GLfloat x, GLfloat y, GLfloat z){
    x *= PI / 180.0;
    y *= PI / 180.0;
    z *= PI / 180.0;
    GLfloat cx = cosf(x);
    GLfloat sx = sinf(x);
    GLfloat cy = cosf(y);
    GLfloat sy = sinf(y);
    GLfloat cz = cosf(z);
    GLfloat sz = sinf(z);
    GLfloat cxsy = cx * sy;
    GLfloat sxsy = sx * sy;

    out[0]  =   cy * cz;
    out[1]  =  -cy * sz;
    out[2]  =   sy;
    out[3]  =  0.0;

    out[4]  =  cxsy * cz + cx * sz;
    out[5]  = -cxsy * sz + cx * cz;
    out[6]  =  -sx * cy;
    out[7]  =  0.0;

    out[8]  = -sxsy * cz + sx * sz;
    out[9]  =  sxsy * sz + sx * cz;
    out[10] =  cx * cy;
    out[11] =  0.0;

    out[12] =  0.0;
    out[13] =  0.0;
    out[14] =  0.0;
    out[15] =  1.0;
}

__attribute__((unused))
void clone(GLfloat * out, GLfloat * a){
    for(int i = 0; i < 16; i++){
        out[i] = a[i];
    }
}

__attribute__((unused))
void transformVec4(GLfloat * out, GLfloat * a, GLfloat * m){
    GLfloat x = a[0], y = a[1], z = a[2], w = a[3];
    out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
    out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
    out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
    out[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
}