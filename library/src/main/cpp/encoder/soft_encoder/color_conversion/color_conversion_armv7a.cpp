#include "color_conversion.h"
#include <arm_neon.h>

#define LOG_TAG "ColorConversionArmv7a"

void yuv420p_to_nv12(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]) {
	uint8_t *yLine = (uint8_t *) srcSlice[0];
	int y_Pitch = srcStride[0];
	uint8_t *uLine = (uint8_t *) srcSlice[1];
	int u_Pitch = srcStride[1];
	uint8_t *vLine = (uint8_t *) srcSlice[2];
	int v_Pitch = srcStride[2];

	uint8_t *y_nvLine = (uint8_t *) dst[0];
	int y_nvPitch = dstStride[0];

	uint8_t *uv_nvLine = (uint8_t *) dst[1];
	int uv_nvPitch = dstStride[1];

	const unsigned int linePairs = (imageHeight) / 2;

	const unsigned int groups = imageWidth / 32;
	const unsigned int tails = imageWidth & 31;
	const unsigned int tailPairs = tails / 2;

	for (unsigned int row = 0; row < linePairs; row++) {
		uint8_t *yPtr = yLine;
		uint8_t *uPtr = uLine;
		uint8_t *vPtr = vLine;

		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *uv_nvPtr = uv_nvLine;

		// copy the y
		memcpy(y_nvPtr, yPtr, imageWidth * sizeof(uint8_t));
		y_nvPtr += y_nvPitch;
		yPtr += y_Pitch;
		memcpy(y_nvPtr, yPtr, imageWidth * sizeof(uint8_t));

		// copy the u v
		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16_t srcU = vld1q_u8(uPtr);
			const uint8x16_t srcV = vld1q_u8(vPtr);

			uint8x16x2_t srcUV;
			srcUV.val[0] = srcU;
			srcUV.val[1] = srcV;

			vst2q_u8(uv_nvPtr, srcUV);

			uPtr += 16;
			vPtr += 16;
			uv_nvPtr += 32;
		}

		for (unsigned int i = 0; i < tailPairs; i++) {
			*(uv_nvPtr++) = *(uPtr++);
			*(uv_nvPtr++) = *(vPtr++);
		}

		if (tails & 1) {
			uv_nvPtr[0] = uPtr[0];
			uv_nvPtr[1] = vPtr[0];
		}

		yLine += y_Pitch;
		yLine += y_Pitch;
		uLine += u_Pitch;
		vLine += v_Pitch;

		y_nvLine += y_nvPitch;
		y_nvLine += y_nvPitch;
		uv_nvLine += uv_nvPitch;
	}

	if (imageHeight & 1) {
		uint8_t *yPtr = yLine;
		uint8_t *uPtr = uLine;
		uint8_t *vPtr = vLine;

		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *uv_nvPtr = uv_nvLine;

		// copy the y
		memcpy(y_nvPtr, yPtr, imageWidth * sizeof(uint8_t));

		// copy the u v
		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16_t srcU = vld1q_u8(uPtr);
			const uint8x16_t srcV = vld1q_u8(vPtr);

			uint8x16x2_t srcUV;
			srcUV.val[0] = srcU;
			srcUV.val[1] = srcV;

			vst2q_u8(uv_nvPtr, srcUV);

			uPtr += 16;
			vPtr += 16;
			uv_nvPtr += 32;
		}

		for (unsigned int i = 0; i < tailPairs; i++) {
			*(uv_nvPtr++) = *(uPtr++);
			*(uv_nvPtr++) = *(vPtr++);
		}

		if (tails & 1) {
			uv_nvPtr[0] = uPtr[0];
			uv_nvPtr[1] = vPtr[0];
		}
	}
}

void yuy2_to_nv12(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]) {
	const uint8_t *yuy2Line = (const uint8_t *) srcSlice[0];
	const int yuy2Pitch = srcStride[0];

	uint8_t *y_nvLine = (uint8_t *) dst[0];
	int y_nvPitch = dstStride[0];

	uint8_t *uv_nvLine = (uint8_t *) dst[1];
	int uv_nvPitch = dstStride[1];

	const unsigned int linePairs = (imageHeight) / 2;

	const unsigned int groups = imageWidth / 32;
	const unsigned int tails = imageWidth & 31;
	const unsigned int tailPairs = tails / 2;
	for (unsigned int row = 0; row < linePairs; row++) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *uv_nvPtr = uv_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];

			vst2q_u8(y_nvPtr, srcY);
			yuy2Ptr += 64;
			y_nvPtr += 32;

			uint8x16x2_t srcUV;
			srcUV.val[0] = srcYUYV.val[1];
			srcUV.val[1] = srcYUYV.val[3];
			vst2q_u8(uv_nvPtr, srcUV);
			uv_nvPtr += 32;
		}

		for (unsigned int i = 0; i < tailPairs; i++) {
			y_nvPtr[0] = yuy2Ptr[0];
			y_nvPtr[1] = yuy2Ptr[2];
			*(uv_nvPtr++) = yuy2Ptr[1];
			*(uv_nvPtr++) = yuy2Ptr[3];

			yuy2Ptr += 4;
			y_nvPtr += 2;
		}
		if (tails & 1) {
			y_nvPtr[0] = yuy2Ptr[0];
			uv_nvPtr[0] = yuy2Ptr[1];
			uv_nvPtr[1] = yuy2Ptr[3];
		}

		yuy2Line += yuy2Pitch;
		y_nvLine += y_nvPitch;
		uv_nvLine += uv_nvPitch;

		// process other line y
		yuy2Ptr = (const uint8_t *) yuy2Line;
		y_nvPtr = y_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];
			vst2q_u8(y_nvPtr, srcY);
			yuy2Ptr += 64;
			y_nvPtr += 32;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			y_nvPtr[0] = yuy2Ptr[0];
			y_nvPtr[1] = yuy2Ptr[2];
			yuy2Ptr += 4;
			y_nvPtr += 2;
		}
		if (tails & 1)
			y_nvPtr[0] = yuy2Ptr[0];

		yuy2Line += yuy2Pitch;
		y_nvLine += y_nvPitch;
	}

	if (imageHeight & 1) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *uv_nvPtr = uv_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];

			vst2q_u8(y_nvPtr, srcY);
			yuy2Ptr += 64;
			y_nvPtr += 32;

			uint8x16x2_t srcUV;
			srcUV.val[0] = srcYUYV.val[1];
			srcUV.val[1] = srcYUYV.val[3];
			vst2q_u8(uv_nvPtr, srcUV);
			uv_nvPtr += 32;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			y_nvPtr[0] = yuy2Ptr[0];
			y_nvPtr[1] = yuy2Ptr[2];
			*(uv_nvPtr++) = yuy2Ptr[1];
			*(uv_nvPtr++) = yuy2Ptr[3];
			yuy2Ptr += 4;
			y_nvPtr += 2;
		}
		if (tails & 1) {
			y_nvPtr[0] = yuy2Ptr[0];
			uv_nvPtr[0] = yuy2Ptr[1];
			uv_nvPtr[1] = yuy2Ptr[3];
		}
	}
}

void yuy2_to_nv21(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]) {
	const uint8_t *yuy2Line = (const uint8_t *) srcSlice[0];
	const int yuy2Pitch = srcStride[0];

	uint8_t *y_nvLine = (uint8_t *) dst[0];
	int y_nvPitch = dstStride[0];

	uint8_t *vu_nvLine = (uint8_t *) dst[1];
	int vu_nvPitch = dstStride[1];

	const unsigned int linePairs = (imageHeight) / 2;

	const unsigned int groups = imageWidth / 32;
	const unsigned int tails = imageWidth & 31;
	const unsigned int tailPairs = tails / 2;
	for (unsigned int row = 0; row < linePairs; row++) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *vu_nvPtr = vu_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];

			vst2q_u8(y_nvPtr, srcY);
			yuy2Ptr += 64;
			y_nvPtr += 32;

			uint8x16x2_t srcVU;
			srcVU.val[0] = srcYUYV.val[3];
			srcVU.val[1] = srcYUYV.val[1];
			vst2q_u8(vu_nvPtr, srcVU);
			vu_nvPtr += 32;
		}

		for (unsigned int i = 0; i < tailPairs; i++) {
			y_nvPtr[0] = yuy2Ptr[0];
			y_nvPtr[1] = yuy2Ptr[2];
			*(vu_nvPtr++) = yuy2Ptr[3];
			*(vu_nvPtr++) = yuy2Ptr[1];

			yuy2Ptr += 4;
			y_nvPtr += 2;
		}
		if (tails & 1) {
			y_nvPtr[0] = yuy2Ptr[0];
			vu_nvPtr[0] = yuy2Ptr[3];
			vu_nvPtr[1] = yuy2Ptr[1];
		}

		yuy2Line += yuy2Pitch;
		y_nvLine += y_nvPitch;
		vu_nvLine += vu_nvPitch;

		// process other line y
		yuy2Ptr = (const uint8_t *) yuy2Line;
		y_nvPtr = y_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];
			vst2q_u8(y_nvPtr, srcY);
			yuy2Ptr += 64;
			y_nvPtr += 32;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			y_nvPtr[0] = yuy2Ptr[0];
			y_nvPtr[1] = yuy2Ptr[2];
			yuy2Ptr += 4;
			y_nvPtr += 2;
		}
		if (tails & 1)
			y_nvPtr[0] = yuy2Ptr[0];

		yuy2Line += yuy2Pitch;
		y_nvLine += y_nvPitch;
	}

	if (imageHeight & 1) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *vu_nvPtr = vu_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];

			vst2q_u8(y_nvPtr, srcY);
			yuy2Ptr += 64;
			y_nvPtr += 32;

			uint8x16x2_t srcVU;
			srcVU.val[0] = srcYUYV.val[3];
			srcVU.val[1] = srcYUYV.val[1];
			vst2q_u8(vu_nvPtr, srcVU);
			vu_nvPtr += 32;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			y_nvPtr[0] = yuy2Ptr[0];
			y_nvPtr[1] = yuy2Ptr[2];
			*(vu_nvPtr++) = yuy2Ptr[3];
			*(vu_nvPtr++) = yuy2Ptr[1];
			yuy2Ptr += 4;
			y_nvPtr += 2;
		}
		if (tails & 1) {
			y_nvPtr[0] = yuy2Ptr[0];
			vu_nvPtr[0] = yuy2Ptr[3];
			vu_nvPtr[1] = yuy2Ptr[1];
		}
	}
}

void yuy2_to_yuv420p(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[],
		const int dstStride[]) {
	const uint8_t *yuy2Line = (const uint8_t *) srcSlice[0];
	const int yuy2Pitch = srcStride[0];
	uint8_t *yLine = (uint8_t *) dst[0];
	const int yPitch = dstStride[0];
	uint8_t *uLine = (uint8_t *) dst[1];
	const int uPitch = dstStride[1];
	uint8_t *vLine = (uint8_t *) dst[2];
	const int vPitch = dstStride[2];
	const unsigned int linePairs = imageHeight / 2;

	const unsigned int groups = imageWidth / 32;
	const unsigned int tails = imageWidth & 31;
	const unsigned int tailPairs = tails / 2;
	for (unsigned int row = 0; row < linePairs; row++) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *yPtr = yLine;
		uint8_t *uPtr = uLine;
		uint8_t *vPtr = vLine;
		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];
			vst2q_u8(yPtr, srcY);
			vst1q_u8(uPtr, srcYUYV.val[1]);
			vst1q_u8(vPtr, srcYUYV.val[3]);
			yuy2Ptr += 64;
			yPtr += 32;
			uPtr += 16;
			vPtr += 16;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			yPtr[0] = yuy2Ptr[0];
			yPtr[1] = yuy2Ptr[2];
			*(uPtr++) = yuy2Ptr[1];
			*(vPtr++) = yuy2Ptr[3];
			yuy2Ptr += 4;
			yPtr += 2;
		}
		if (tails & 1) {
			yPtr[0] = yuy2Ptr[0];
			uPtr[0] = yuy2Ptr[1];
			vPtr[0] = yuy2Ptr[3];
		}

		yuy2Line += yuy2Pitch;
		yLine += yPitch;
		uLine += uPitch;
		vLine += vPitch;

		yuy2Ptr = (const uint8_t *) yuy2Line;
		yPtr = yLine;
		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];
			vst2q_u8(yPtr, srcY);
			yuy2Ptr += 64;
			yPtr += 32;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			yPtr[0] = yuy2Ptr[0];
			yPtr[1] = yuy2Ptr[2];
			yuy2Ptr += 4;
			yPtr += 2;
		}
		if (tails & 1)
			yPtr[0] = yuy2Ptr[0];

		yuy2Line += yuy2Pitch;
		yLine += yPitch;
	}

	if (imageHeight & 1) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *yPtr = yLine;
		uint8_t *uPtr = uLine;
		uint8_t *vPtr = vLine;
		for (unsigned int i = 0; i < groups; i++) {
			const uint8x16x4_t srcYUYV = vld4q_u8(yuy2Ptr);
			uint8x16x2_t srcY;
			srcY.val[0] = srcYUYV.val[0];
			srcY.val[1] = srcYUYV.val[2];
			vst2q_u8(yPtr, srcY);
			vst1q_u8(uPtr, srcYUYV.val[1]);
			vst1q_u8(vPtr, srcYUYV.val[3]);
			yuy2Ptr += 64;
			yPtr += 32;
			uPtr += 16;
			vPtr += 16;
		}
		for (unsigned int i = 0; i < tailPairs; i++) {
			yPtr[0] = yuy2Ptr[0];
			yPtr[1] = yuy2Ptr[2];
			*(uPtr++) = yuy2Ptr[1];
			*(vPtr++) = yuy2Ptr[3];
			yuy2Ptr += 4;
			yPtr += 2;
		}
		if (tails & 1) {
			yPtr[0] = yuy2Ptr[0];
			uPtr[0] = yuy2Ptr[1];
			vPtr[0] = yuy2Ptr[3];
		}
	}
}
