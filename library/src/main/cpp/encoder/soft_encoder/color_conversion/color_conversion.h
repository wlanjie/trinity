#ifndef COLOR_CONVERSION_H_
#define COLOR_CONVERSION_H_

#include "CommonTools.h"

void yuv420p_to_nv12(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]);

void yuy2_to_nv12(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]);

void yuy2_to_nv21(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]);

void yuy2_to_yuv420p(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]);

#endif
