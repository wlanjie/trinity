//
//  mesh_factory.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include <malloc.h>
#include <string.h>
#include "mesh_factory.h"
#include "math.h"

Mesh *get_ball_mesh() {
    Mesh *ball_mesh;
    ball_mesh = (Mesh *) malloc(sizeof(Mesh));
    memset(ball_mesh, 0, sizeof(Mesh));
    int i, j, ii, jj;
    int step = 5;
    int jstep = 360 / step + 1;
    ball_mesh->ppLen = (180 / step + 1) * (360 / step + 1) * 3;
    ball_mesh->ttLen = (180 / step + 1) * (360 / step + 1) * 2;
    ball_mesh->indexLen = (360 / step) * (180 / step) * 6;
    ball_mesh->pp = (float *) malloc((size_t) (sizeof(float) * ball_mesh->ppLen));
    ball_mesh->tt = (float *) malloc((size_t) (sizeof(float) * ball_mesh->ttLen));
    ball_mesh->index = (unsigned int *) malloc(sizeof(unsigned int) * ball_mesh->indexLen);

    for (j = -90, jj = 90; j <= jj; j += step) {
        for (i = 0, ii = 360; i <= ii; i += step) {
            float sini = sinf((float)i / 180.0f * (float)M_PI);
            float cosi = cosf((float)i / 180.0f * (float)M_PI);
            float sinj = sinf((float)j / 180.0f * (float)M_PI);
            float cosj = cosf((float)j / 180.0f * (float)M_PI);
            float r = 1;
            float y = r * sinj;
            float x = r * cosj * cosi;
            float z = r * cosj * sini;
            *ball_mesh->pp++ = x;
            *ball_mesh->pp++ = y;
            *ball_mesh->pp++ = z;
            *ball_mesh->tt++ = (float)i / 360.0f;
            *ball_mesh->tt++ = (float)j / 90.0f / 2.0f + 0.5f;
        }
    }

    for (i = 0, ii = 360 / step; i < ii; ++i) {
        for (j = 0, jj = 180 / step; j < jj; j++) {
            *ball_mesh->index++ = (unsigned int) (j * jstep + i);
            *ball_mesh->index++ = (unsigned int) (j * jstep + i + 1);
            *ball_mesh->index++ = (unsigned int) ((j + 1) * jstep + i + 1);
            *ball_mesh->index++ = (unsigned int) (j * jstep + i);
            *ball_mesh->index++ = (unsigned int) ((j + 1) * jstep + i + 1);
            *ball_mesh->index++ = (unsigned int) ((j + 1) * jstep + i);
        }
    }
    ball_mesh->pp -= ball_mesh->ppLen;
    ball_mesh->tt -= ball_mesh->ttLen;
    ball_mesh->index -= ball_mesh->indexLen;
    return ball_mesh;
}

Mesh *get_rect_mesh() {
    Mesh *rect_mesh;
    rect_mesh = (Mesh *) malloc(sizeof(Mesh));
    memset(rect_mesh, 0, sizeof(Mesh));

    rect_mesh->ppLen = 12;
    rect_mesh->ttLen = 8;
    rect_mesh->indexLen = 6;
    rect_mesh->pp = (float *) malloc((size_t) (sizeof(float) * rect_mesh->ppLen));
    rect_mesh->tt = (float *) malloc((size_t) (sizeof(float) * rect_mesh->ttLen));
    rect_mesh->index = (unsigned int *) malloc(sizeof(unsigned int) * rect_mesh->indexLen);
    float pa[12] = {-1, 1, -0.1f, 1, 1, -0.1f, 1, -1, -0.1f, -1, -1, -0.1f};
    float ta[8] = {0, 1, 1, 1, 1, 0, 0, 0};
    unsigned int ia[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 12; ++i) {
        *rect_mesh->pp++ = pa[i];
    }
    for (int i = 0; i < 8; ++i) {
        *rect_mesh->tt++ = ta[i];
    }
    for (int i = 0; i < 6; ++i) {
        *rect_mesh->index++ = ia[i];
    }
    rect_mesh->pp -= rect_mesh->ppLen;
    rect_mesh->tt -= rect_mesh->ttLen;
    rect_mesh->index -= rect_mesh->indexLen;
    return rect_mesh;
}

Mesh *get_planet_mesh() {
    Mesh *planet_mesh;
    planet_mesh = (Mesh *) malloc(sizeof(Mesh));
    memset(planet_mesh, 0, sizeof(Mesh));
    int i, j, ii, jj;
    int step = 1;
    float sinj, cosj;
    int jstep = 360 / step + 1;
    planet_mesh->ppLen = (180 / step + 1) * (360 / step + 1) * 3;
    planet_mesh->ttLen = (180 / step + 1) * (360 / step + 1) * 2;
    planet_mesh->indexLen = (360 / step) * (180 / step) * 6;
    planet_mesh->pp = (float *) malloc((size_t) (sizeof(float) * planet_mesh->ppLen));
    planet_mesh->tt = (float *) malloc((size_t) (sizeof(float) * planet_mesh->ttLen));
    planet_mesh->index = (unsigned int *) malloc(sizeof(unsigned int) * planet_mesh->indexLen);

    for (i = 0, ii = 180; i <= ii; i += step) {
        for (j = 0, jj = 360; j <= jj; j += step) {
            sinj = sinf((float)j / 180.0f * (float)M_PI);
            cosj = cosf((float)j / 180.0f * (float)M_PI);
            *planet_mesh->pp++ = (float)i * cosj / 180.0f;
            *planet_mesh->pp++ = (float)i * sinj / 180.0f;
            *planet_mesh->pp++ = -1.0f;
            *planet_mesh->tt++ = 1.0f - (float)j / 360.0f;
            *planet_mesh->tt++ = (float)i / 180.0f;
        }
    }

    for (j = 0, jj = 180 / step; j < jj; ++j) {
        for (i = 0, ii = 360 / step; i < ii; ++i) {
            *planet_mesh->index++ = (unsigned int) (j * jstep + i);
            *planet_mesh->index++ = (unsigned int) ((j + 1) * jstep + i);
            *planet_mesh->index++ = (unsigned int) ((j + 1) * jstep + i + 1);

            *planet_mesh->index++ = (unsigned int) (j * jstep + i);
            *planet_mesh->index++ = (unsigned int) ((j + 1) * jstep + i + 1);
            *planet_mesh->index++ = (unsigned int) (j * jstep + i + 1);
        }
    }

    planet_mesh->pp -= planet_mesh->ppLen;
    planet_mesh->tt -= planet_mesh->ttLen;
    planet_mesh->index -= planet_mesh->indexLen;
    return planet_mesh;
}


static inline float distortionFactor(float radius) {
    int s_numberOfCoefficients = 2;
    static float _coefficients[2] = {0.441000015f, 0.156000003f};

    float result = 1.0f;
    float rFactor = 1.0f;
    float squaredRadius = radius * radius;
    for (int i = 0; i < s_numberOfCoefficients; i++) {
        rFactor *= squaredRadius;
        result += _coefficients[i] * rFactor;
    }
    return result;
}

static inline float distort(float radius){
    return radius * distortionFactor(radius);
}

static inline float blueDistortInverse(float radius) {
    float r0 = radius / 0.9f;
    float r = radius * 0.9f;
    float dr0 = radius - distort(r0);
    while (fabsf(r - r0) > 0.0001f) {
        float dr = radius - distort(r);
        float r2 = r - dr * ((r - r0) / (dr - dr0));
        r0 = r;
        r = r2;
        dr0 = dr;
    }
    return r;
}
static inline float clamp(float val, float min, float max)
{
    float temp = val > min ? val : min;
    return temp < max ? temp : max;
}

// eye : 0 left
//       1 right
Mesh * get_distortion_mesh(int eye){
    Mesh *distortion_mesh = (Mesh *) malloc(sizeof(Mesh));
    float xEyeOffsetScreen = 0.523064613f;
    float yEyeOffsetScreen = 0.80952388f;

    float viewportWidthTexture = 1.43138313f;
    float viewportHeightTexture = 1.51814604f;

    float viewportXTexture = 0;
    float viewportYTexture = 0;

    float textureWidth = 2.86276627f;
    float textureHeight = 1.51814604f;

    float xEyeOffsetTexture = 0.592283607f;
    float yEyeOffsetTexture = 0.839099586f;

    float screenWidth = 2.47470069f;
    float screenHeight = 1.39132345f;

    if (eye == 1) {
        xEyeOffsetScreen = 1.95163608f;
        viewportXTexture = 1.43138313f;
        xEyeOffsetTexture = 2.27048278f;
    }

    const int rows = 40;
    const int cols = 40;
//    float vertexData[rows * cols * 9];
    float * vertexData = malloc(sizeof(float) * rows * cols * 9);
    unsigned int vertexOffset = 0;
    const float vignetteSizeTanAngle = 0.05f;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            const float uTextureBlue = col / 39.0f * (viewportWidthTexture / textureWidth) + viewportXTexture / textureWidth;
            const float vTextureBlue = row / 39.0f * (viewportHeightTexture / textureHeight) + viewportYTexture / textureHeight;

            const float xTexture = uTextureBlue * textureWidth - xEyeOffsetTexture;
            const float yTexture = vTextureBlue * textureHeight - yEyeOffsetTexture;
            const float rTexture = sqrtf(xTexture * xTexture + yTexture * yTexture);

            const float textureToScreenBlue = (rTexture > 0.0f) ? blueDistortInverse(rTexture) / rTexture : 1.0f;

            const float xScreen = xTexture * textureToScreenBlue;
            const float yScreen = yTexture * textureToScreenBlue;

            const float uScreen = (xScreen + xEyeOffsetScreen) / screenWidth;
            const float vScreen = (yScreen + yEyeOffsetScreen) / screenHeight;
            const float rScreen = rTexture * textureToScreenBlue;

            const float screenToTextureGreen = (rScreen > 0.0f) ? distortionFactor(rScreen) : 1.0f;
            const float uTextureGreen = (xScreen * screenToTextureGreen + xEyeOffsetTexture) / textureWidth;
            const float vTextureGreen = (yScreen * screenToTextureGreen + yEyeOffsetTexture) / textureHeight;

            const float screenToTextureRed = (rScreen > 0.0f) ? distortionFactor(rScreen) : 1.0f;
            const float uTextureRed = (xScreen * screenToTextureRed + xEyeOffsetTexture) / textureWidth;
            const float vTextureRed = (yScreen * screenToTextureRed + yEyeOffsetTexture) / textureHeight;

            const float vignetteSizeTexture = vignetteSizeTanAngle / textureToScreenBlue;

            const float dxTexture = xTexture + xEyeOffsetTexture - clamp(xTexture + xEyeOffsetTexture,
                                                                         viewportXTexture + vignetteSizeTexture,
                                                                         viewportXTexture + viewportWidthTexture - vignetteSizeTexture);
            const float dyTexture = yTexture + yEyeOffsetTexture - clamp(yTexture + yEyeOffsetTexture,
                                                                         viewportYTexture + vignetteSizeTexture,
                                                                         viewportYTexture + viewportHeightTexture - vignetteSizeTexture);
            const float drTexture = sqrtf(dxTexture * dxTexture + dyTexture * dyTexture);

            float vignette = 1.0f - clamp(drTexture / vignetteSizeTexture, 0.0f, 1.0f);

            vertexData[(vertexOffset + 0)] = 2.0f * uScreen - 1.0f;
            vertexData[(vertexOffset + 1)] = 2.0f * vScreen - 1.0f;
            vertexData[(vertexOffset + 2)] = vignette;
            vertexData[(vertexOffset + 3)] = uTextureRed;
            vertexData[(vertexOffset + 4)] = vTextureRed;
            vertexData[(vertexOffset + 5)] = uTextureGreen;
            vertexData[(vertexOffset + 6)] = vTextureGreen;
            vertexData[(vertexOffset + 7)] = uTextureBlue;
            vertexData[(vertexOffset + 8)] = vTextureBlue;

            vertexOffset += 9;
        }
    }


    int index_count = (rows - 1) * (cols - 1) * 6;
    unsigned int * indexData = malloc(sizeof(unsigned int) * index_count);
    for(int i = 0; i < rows - 1; ++i){
        for(int j = 0; j < cols - 1; ++j){
            *indexData++ = (unsigned int) (j * cols + i);
            *indexData++ = (unsigned int) ((j + 1) * cols + i);
            *indexData++ = (unsigned int) ((j + 1) * cols + i + 1);

            *indexData++ = (unsigned int) (j * cols + i);
            *indexData++ = (unsigned int) ((j + 1) * cols + i + 1);
            *indexData++ = (unsigned int) (j * cols + i + 1);
        }
    }
    indexData -= index_count;

    distortion_mesh->pp = vertexData;
    distortion_mesh->ppLen = rows * cols * 9;
    distortion_mesh->tt = NULL;
    distortion_mesh->index = indexData;
    distortion_mesh->indexLen = index_count;
    return distortion_mesh;
}


void free_mesh(Mesh *p) {
    if (p != NULL) {
        if (p->index != NULL) {
            free(p->index);
        }
        if (p->pp != NULL) {
            free(p->pp);
        }
        if (p->tt != NULL) {
            free(p->tt);
        }
        free(p);
    }
}