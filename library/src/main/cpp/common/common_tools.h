#ifndef SONG_STUDIO_COMMON
#define SONG_STUDIO_COMMON
#include <jni.h>
#include <android/log.h> 
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define UINT64_C        uint64_t
#define INT16_MAX        32767
#define INT16_MIN       -32768

#define LOG "mv"

#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG, __VA_ARGS__)
typedef signed short SInt16;
typedef unsigned char byte;
#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))
#define AUDIO_PCM_OUTPUT_CHANNEL 2
#define ACCOMPANY_PCM_OUTPUT_CHANNEL 2

static inline char* intToStr(int paramIntValue) {
//	int radix = 10;
//	unsigned char table[] = "0123456789";
//	// 逆序保存
//	bool isNegtive = false;
//	if (paramIntValue < 0) {
//		paramIntValue = paramIntValue * -1;
//		isNegtive = true;
//	}
//	int tmp = paramIntValue;
//	char paramTarget[33];
//	char *p = &paramTarget[32];
//	*p-- = '\0';
//	int length = 1;
//	do {
//		*p-- = table[tmp % radix];
//		tmp /= radix;
//		length++;
//	} while (tmp > 0);
//	if (isNegtive) {
//		*p-- = '-';
//	}
//	//重新分配空间 并且从p位置，因为上边保存到得位置就是0
//	char* result = new char[length];
//	for (int i = 0; i < length; i++) {
//		result[i] = *(p + 1 + i);
//	}
	char* result = new char[10];
	sprintf(result, "%d", paramIntValue);
	return result;
}

static inline int getIndexOfMaxValueInArray(long* array, int length) {
	if (length <= 0) {
		return -1;
	}
	int result = 0;
	long max = array[0];
	for (int i = 0; i < length - 1; i++) {
		if (array[i] < array[i + 1]) {
			max = array[i + 1];
			result = i + 1;
		}
	}
	return result;
}

static inline bool isAACSuffix(const char* accompanyPath){
	bool ret = false;
	const char *suffix = strrchr(accompanyPath, '.');
//	LOGI("suffix is %s", suffix);
	if (0 == strcmp(suffix, ".aac")) {
		ret = true;
	}
	return ret;
}

static inline bool isPNGSuffix(const char* picFilePath){
	bool ret = false;
	const char *suffix = strrchr(picFilePath, '.');
//	LOGI("suffix is %s", suffix);
	if (0 == strcmp(suffix, ".png")) {
		ret = true;
	}
	return ret;
}

static inline long getCurrentTime()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline long long currentTimeMills(){
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline long getCurrentTimeSecSinceReferenceDate()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec;
}
inline int readShortFromFile(short *shortarray, int size, FILE* fp) {
	int actualSize = fread(shortarray, 2, size, fp);
	//	LOGI("read data expected Size is %d and Size is %d", Size, actualSize);
	if (actualSize != size) {
//		__android_log_print(ANDROID_LOG_INFO, "COMMON_TOOLS", "actualSize is %d Size is %d", actualSize, Size);
		int flag = feof(fp);
		if (flag != 0 && actualSize == 0) {
			//到达文件末尾
//			__android_log_print(ANDROID_LOG_INFO, "COMMON_TOOLS", "end of file, actualSize is %d", actualSize);
			return -1;
		}
	}
	return actualSize;
}

inline int readByteFromFile(byte *bytearray, int size, FILE* fp) {
	int actualSize = fread(bytearray, 1, size, fp);
	//	LOGI("read data expected Size is %d and Size is %d", Size, actualSize);
	if (actualSize != size) {
		int flag = feof(fp);
		if (flag != 0 && actualSize == 0) {
			//到达文件末尾
			//			LOGI("end of file");
			return -1;
		}
	}
	return actualSize;
}

//合并两个short，返回一个short
inline SInt16 TPMixSamples(SInt16 a, SInt16 b) {
	int tmp = a < 0 && b < 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MIN) : (a > 0 && b > 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MAX) : a + b);
	return tmp > INT16_MAX ? INT16_MAX : (tmp < INT16_MIN ? INT16_MIN : tmp);
}

//合并两个float，返回一个short
inline SInt16 TPMixSamplesFloat(float a, float b) {
	int tmp = a < 0 && b < 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MIN) : (a > 0 && b > 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MAX) : a + b);
	return tmp > INT16_MAX ? INT16_MAX : (tmp < INT16_MIN ? INT16_MIN : tmp);
}

//把一个short转换为一个长度为2的byte数组
inline void converttobytearray(SInt16 source, byte* bytes2) {
	bytes2[0] = (byte) (source & 0xff);
	bytes2[1] = (byte) ((source >> 8) & 0xff);
}

//将两个byte转换为一个short
inline SInt16 convertshort(byte* bytes) {
	return (bytes[0] << 8) + (bytes[1] & 0xFF);
}

//调节音量的方法
inline SInt16 adjustAudioVolume(SInt16 source, float volume) {
	SInt16 result = source;
	int temp = (int) ((int) source * volume);
	if (temp < -0x8000) {
		result = -0x8000;
	} else if (temp > 0x7FFF) {
		result = 0x7FFF;
	} else {
		result = (short) temp;
	}
	return result;
}

//将一个short数组转换为一个byte数组---清唱时由于不需要和伴奏合成，所以直接转换;还有一个是当解码完成之后，需要将short变为byte数组，写入文件
inline void convertByteArrayFromShortArray(SInt16 *shortarray, int size, byte *bytearray) {
	byte* tmpbytearray = new byte[2];
	for (int i = 0; i < size; i++) {
		converttobytearray(shortarray[i], tmpbytearray);
		bytearray[i * 2] = tmpbytearray[0];
		bytearray[i * 2 + 1] = tmpbytearray[1];
	}
	delete[] tmpbytearray;
}

//将一个byte数组转换为一个short数组---读进来audioRecord录制的pcm，然后将其从文件中读出字节流，然后在转换为short以方便后续处理
inline void convertShortArrayFromByteArray(byte *bytearray, int size, SInt16 *shortarray, float audioVolume) {
	byte* bytes = new byte[2];
	for (int i = 0; i < size / 2; i++) {
		bytes[1] = bytearray[2 * i];
		bytes[0] = bytearray[2 * i + 1];
		SInt16 source = convertshort(bytes);
		if (audioVolume != 1.0) {
			shortarray[i] = adjustAudioVolume(source, audioVolume);
		} else {
			shortarray[i] = source;
		}
	}
	delete[] bytes;
}

//客户端代码需要根据accompanySampleRate / audioSampleRate算出transfer_ratio;
//以及根据(int)((float)(sample_count) / accompany_sample_rate_ * sample_rate_)算出transfered_sample_count;
//并且分配出samples_transfered---将伴奏mp3解析成为pcm的short数组，需要先进行转换为录音的采样频率
inline void convertAccompanySampleRateByAudioSampleRate(SInt16 *samples, SInt16 *samples_transfered, int transfered_sample_count, float transfer_ratio) {
	for (int i = 0; i < transfered_sample_count; i++) {
		samples_transfered[i] = samples[(int) (i * transfer_ratio)];
	}
}

//调节采样的音量---非清唱时候最终合成伴奏与录音的时候，判断如果accompanyVolume不是1.0的话，先调节伴奏的音量；而audioVolume是在读出的字节流转换为short流的时候调节的。
inline void adjustSamplesVolume(SInt16 *samples, int size, float accompanyVolume) {
	if (accompanyVolume != 1.0) {
		for (int i = 0; i < size; i++) {
			samples[i] = adjustAudioVolume(samples[i], accompanyVolume);
		}
	}
}

//合成伴奏与录音,byte数组需要在客户端分配好内存---非清唱时候最终合成伴奏与录音调用
inline void mixtureAccompanyAudio(SInt16 *accompanyData, SInt16 *audioData, int size, byte *targetArray) {
	byte* tmpbytearray = new byte[2];
	for (int i = 0; i < size; i++) {
		SInt16 audio = audioData[i];
		SInt16 accompany = accompanyData[i];
		SInt16 temp = TPMixSamples(accompany, audio);
		converttobytearray(temp, tmpbytearray);
		targetArray[i * 2] = tmpbytearray[0];
		targetArray[i * 2 + 1] = tmpbytearray[1];
	}
	delete[] tmpbytearray;
}

//合成伴奏与录音,short数组需要在客户端分配好内存---非清唱时候边和边放调用
inline void mixtureAccompanyAudio(SInt16 *accompanyData, SInt16 *audioData, int size, short *targetArray) {
	for (int i = 0; i < size; i++) {
		SInt16 audio = audioData[i];
		SInt16 accompany = accompanyData[i];
		targetArray[i] = TPMixSamples(accompany, audio);
	}
}


inline char *strstr(char *s1, char *s2) {
	unsigned int i = 0;
	if (*s1 == 0) // 如果字符串s1为空
			{
		if (*s2) // 如果字符串s2不为空
			return (char*) NULL; // 则返回NULL
		return (char*) s1; // 如果s2也为空，则返回s1
	}

	while (*s1) // 串s1没有结束
	{
		i = 0;
		while (1) {
			if (s2[i] == 0) {
				return (char*) s1;
			}
			if (s2[i] != s1[i])
				break;
			i++;
		}
		s1++;
	}
	return (char*) NULL;
}

inline int strindex(char *s1, char *s2) {
	int nPos = -1;
	char *res = strstr(s1, s2);
	if (res)
		nPos = res - s1;
	return nPos;
}

#endif //SONG_STUDIO_COMMON
