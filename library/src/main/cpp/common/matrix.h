#ifndef MATRIX_H
#define MATRIX_H

#include <math.h>
#include <stdlib.h>
#include <string>

void matrixSetIdentityM(float *m);
void matrixSetRotateM(float *m, float a, float x, float y, float z);
void matrixMultiplyMM(float *m, float *lhs, float *rhs);
void matrixScaleM(float *m, float x, float y, float z);
void matrixTranslateM(float *m, float x, float y, float z);
void matrixRotateM(float *m, float a, float x, float y, float z);
void matrixLookAtM(float *m, float eyeX, float eyeY, float eyeZ, float cenX,
		float cenY, float cenZ, float upX, float upY, float upZ);
void matrixFrustumM(float *m, float left, float right, float bottom, float top, float near, float far);

void getTranslateMatrix(float *m, float x, float y, float z);

#endif // MATRIX_H
