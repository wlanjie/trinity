#include "color_conversion.h"
#include <emmintrin.h>
#define LOG_TAG "ColorConversionX86"

void yuy2_to_nv12(const uint8_t * const srcSlice[], const int srcStride[],
		int imageWidth, int imageHeight, uint8_t * const dst[], const int dstStride[]) {
	const uint8_t *yuy2Line = (const uint8_t *) srcSlice[0];
	const int yuy2Pitch = srcStride[0];

	uint8_t *y_nvLine = (uint8_t *) dst[0];
	int y_nvPitch = dstStride[0];

	uint8_t *uv_nvLine = (uint8_t *) dst[1];
	int uv_nvPitch = dstStride[1];

	const unsigned int linePairs = (imageHeight) / 2;

	__m128i xmm0, xmm1, xmm2;

	const unsigned int groups = imageWidth / 16;
	const unsigned int tails = imageWidth & 15;
	const unsigned int tailPairs = tails / 2;

	for (unsigned int row = 0; row < linePairs; row++) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *uv_nvPtr = uv_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | u14| v12 | u12| v10 | u10| v8  | u8 | v6  | u6 | v4  | u4 | v2 | u2 | v0 | u0] */

			_mm_storeu_si128((__m128i *) y_nvPtr, xmm2);
			_mm_storeu_si128((__m128i *) uv_nvPtr, xmm0);

			yuy2Ptr += 32;
			y_nvPtr += 16;
			uv_nvPtr += 16;
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
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */

			_mm_storeu_si128((__m128i *) y_nvPtr, xmm2);

			yuy2Ptr += 32;
			y_nvPtr += 16;
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
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | u14| v12 | u12| v10 | u10| v8  | u8 | v6  | u6 | v4  | u4 | v2 | u2 | v0 | u0] */

			_mm_storeu_si128((__m128i *) y_nvPtr, xmm2);
			_mm_storeu_si128((__m128i *) uv_nvPtr, xmm0);

			yuy2Ptr += 32;
			y_nvPtr += 16;
			uv_nvPtr += 16;
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

	__m128i xmm0, xmm1, xmm2;

	const unsigned int groups = imageWidth / 16;
	const unsigned int tails = imageWidth & 15;
	const unsigned int tailPairs = tails / 2;

	for (unsigned int row = 0; row < linePairs; row++) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *y_nvPtr = y_nvLine;
		uint8_t *vu_nvPtr = vu_nvLine;

		for (unsigned int i = 0; i < groups; i++) {
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm0); /* [ u14 | v14| u12 | v12| u10 | v10| u8  | v8 | u6  | v6 | u4  | v4 | u2 | v2 | u0 | v0] */

			_mm_storeu_si128((__m128i *) y_nvPtr, xmm2);
			_mm_storeu_si128((__m128i *) vu_nvPtr, xmm1);

			yuy2Ptr += 32;
			y_nvPtr += 16;
			vu_nvPtr += 16;
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
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */

			_mm_storeu_si128((__m128i *) y_nvPtr, xmm2);

			yuy2Ptr += 32;
			y_nvPtr += 16;
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
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm0); /* [ u14 | v14| u12 | v12| u10 | v10| u8  | v8 | u6  | v6 | u4  | v4 | u2 | v2 | u0 | v0] */

			_mm_storeu_si128((__m128i *) y_nvPtr, xmm2);
			_mm_storeu_si128((__m128i *) vu_nvPtr, xmm1);

			yuy2Ptr += 32;
			y_nvPtr += 16;
			vu_nvPtr += 16;
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
		for (unsigned int i = 0; i < imageWidth / 2; i++) {
			uv_nvPtr[0] = uPtr[0];
			uv_nvPtr[1] = vPtr[0];

			uv_nvPtr += 2;

			uPtr++;
			vPtr++;
		}

		if (imageWidth & 1) {
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
		for (unsigned int i = 0; i < imageWidth / 2; i++) {
			uv_nvPtr[0] = uPtr[0];
			uv_nvPtr[1] = vPtr[0];

			uv_nvPtr += 2;

			uPtr++;
			vPtr++;
		}

		if (imageWidth & 1) {
			uv_nvPtr[0] = uPtr[0];
			uv_nvPtr[1] = vPtr[0];
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

	__m128i xmm0, xmm1, xmm2;

	const unsigned int groups = imageWidth / 16;
	const unsigned int tails = imageWidth & 15;
	const unsigned int tailPairs = tails / 2;

	for (unsigned int row = 0; row < linePairs; row++) {
		const uint8_t *yuy2Ptr = (const uint8_t *) yuy2Line;
		uint8_t *yPtr = yLine;
		uint8_t *uPtr = uLine;
		uint8_t *vPtr = vLine;

		for (unsigned int i = 0; i < groups; i++) {
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */
			xmm0 = _mm_srli_si128(xmm0, 8); /* [  0  | 0  | 0   | 0  | 0   | 0  | 0   | 0  | u14 | u12| u10 | u8 | u6 | u4 | u2 | u0] */
			xmm1 = _mm_srli_si128(xmm1, 8); /* [  0  | 0  | 0   | 0  | 0   | 0  | 0   | 0  | v14 | v12| v10 | v8 | v6 | v4 | v2 | v0] */

			_mm_storeu_si128((__m128i *) yPtr, xmm2);
			_mm_storel_epi64((__m128i *) uPtr, xmm0);
			_mm_storel_epi64((__m128i *) vPtr, xmm1);

			yuy2Ptr += 32;
			yPtr += 16;
			uPtr += 8;
			vPtr += 8;
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

		// process other line y
		yuy2Ptr = (const uint8_t *) yuy2Line;
		yPtr = yLine;

		for (unsigned int i = 0; i < groups; i++) {
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */

			_mm_storeu_si128((__m128i *) yPtr, xmm2);

			yuy2Ptr += 32;
			yPtr += 16;
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
			xmm0 = _mm_loadu_si128((__m128i *) yuy2Ptr); /* [ v6  | y7 | u6  | y6 | v4  | y5 | u4  | y4 | v2  | y3 | u2  | y2 | v0 | y1 | u0 | y0] */
			xmm1 = _mm_loadu_si128((__m128i *) yuy2Ptr + 1); /* [ v14 | y15| u14 | y14| v12 | y13| u12 | y12| v10 | y11| u10 | y10| v8 | y9 | u8 | y8] */

			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ v10 | v2 | y11 | y3 | u10 | u2 | y10 | y2 | v8  | v0 | y9  | y1 | u8 | u0 | y8 | y0] */
			xmm0 = _mm_unpackhi_epi8(xmm0, xmm1); /* [ v14 | v6 | y15 | y7 | u14 | u6 | y14 | y6 | v12 | v4 | y13 | y5 | u12| u4 | y12| y4] */
			xmm1 = _mm_unpacklo_epi8(xmm2, xmm0); /* [ v12 | v8 | v4  | v0 | y13 | y9 | y5  | y1 | u12 | u8 | u4  | u0 | y12| y8 | y4 | y0] */
			xmm2 = _mm_unpackhi_epi8(xmm2, xmm0); /* [ v14 | v10| v6  | v2 | y15 | y11| y7  | y3 | u14 | u10| u6  | u2 | y14| y10| y6 | y2] */
			xmm0 = _mm_unpacklo_epi8(xmm1, xmm2); /* [ u14 | u12| u10 | u8 | u6  | u4 | u2  | u0 | y14 | y12| y10 | y8 | y6 | y4 | y2 | y0] */
			xmm1 = _mm_unpackhi_epi8(xmm1, xmm2); /* [ v14 | v12| v10 | v8 | v6  | v4 | v2  | v0 | y15 | y13| y11 | y9 | y7 | y5 | y3 | y1] */
			xmm2 = _mm_unpacklo_epi8(xmm0, xmm1); /* [ y15 | y14| y13 | y12| y11 | y10| y9  | y8 | y7  | y6 | y5  | y4 | y3 | y2 | y1 | y0] */
			xmm0 = _mm_srli_si128(xmm0, 8); /* [  0  | 0  | 0   | 0  | 0   | 0  | 0   | 0  | u14 | u12| u10 | u8 | u6 | u4 | u2 | u0] */
			xmm1 = _mm_srli_si128(xmm1, 8); /* [  0  | 0  | 0   | 0  | 0   | 0  | 0   | 0  | v14 | v12| v10 | v8 | v6 | v4 | v2 | v0] */

			_mm_storeu_si128((__m128i *) yPtr, xmm2);
			_mm_storel_epi64((__m128i *) uPtr, xmm0);
			_mm_storel_epi64((__m128i *) vPtr, xmm1);

			yuy2Ptr += 32;
			yPtr += 16;
			uPtr += 8;
			vPtr += 8;
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
