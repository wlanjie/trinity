#ifndef SONG_STUDIO_WAV_TOOLS_H_
#define SONG_STUDIO_WAV_TOOLS_H_
#include "CommonTools.h"

#define LOG_TAG "SongStudio WaveHeader"

typedef struct WaveHeader {
	long totalAudioLen;
	long totalDataLen;
	long longSampleRate;
	int channels;
	long byteRate;
} WaveHeader;

static inline long convetToLongFromByteArray(byte* buffer) {
	return (int) ((buffer[3] & 0xff) << 24) + (int) ((buffer[2] & 0xff) << 16)
			+ (int) ((buffer[1] & 0xff) << 8) + (int) (buffer[0] & 0xff);
}

static inline void convertToPCMFileFromWavFile(const char* wavFilePath,
		const  char* pcmFilePath) {
	FILE* input_file = NULL;
	FILE* output_file = NULL;
	input_file = fopen(wavFilePath, "rb");
	output_file = fopen(pcmFilePath, "wb+");
	if (NULL != input_file && NULL != output_file) {
		byte* header = new byte[44];
		fread(header, 1, 44, input_file);
		byte* buffer = new byte[1024 * 4];
		int length = -1;
		while ((length = fread(buffer, 1, 1024 * 4, input_file)) > 0) {
			fwrite(buffer, 1, length, output_file);
		}
		fclose(input_file);
		fclose(output_file);
	} else {
		LOGI("input file or output file invalid... \n");
	}
}

static inline WaveHeader* readWaveHeader(const char* wavFilePath) {
	WaveHeader* waveHeader = NULL;
	FILE* input_file = NULL;
	input_file = fopen(wavFilePath, "rb");
	if (NULL != input_file) {
		waveHeader = new WaveHeader();
		byte* header = new byte[44];
		int readLength = fread(header, 1, 44, input_file);
		if (readLength == 44) {
			byte* buffer = new byte[4];
			// totalDataLen
			buffer[0] = header[4];
			buffer[1] = header[5];
			buffer[2] = header[6];
			buffer[3] = header[7];
			waveHeader->totalDataLen = convetToLongFromByteArray(buffer);
			waveHeader->channels = header[22];
			buffer[0] = header[24];
			buffer[1] = header[25];
			buffer[2] = header[26];
			buffer[3] = header[27];
			waveHeader->longSampleRate = convetToLongFromByteArray(buffer);
			buffer[0] = header[28];
			buffer[1] = header[29];
			buffer[2] = header[30];
			buffer[3] = header[31];
			waveHeader->byteRate = convetToLongFromByteArray(buffer);
			buffer[0] = header[40];
			buffer[1] = header[41];
			buffer[2] = header[42];
			buffer[3] = header[43];
			waveHeader->totalAudioLen = convetToLongFromByteArray(buffer);
		}
		fclose(input_file);
	} else {
		LOGI("input file or output file invalid... \n");
	}
	return waveHeader;
}

/**
 * 这里提供一个头信息。插入这些信息就可以得到可以播放的文件。 为我为啥插入这44个字节，这个还真没深入研究，不过你随便打开一个wav
 * 音频的文件，可以发现前面的头文件可以说基本一样哦。每种格式的文件都有 自己特有的头文件。
 */
static inline byte* WriteWaveFileHeader(WaveHeader* waveHeader) {
	byte* header = new byte[44];
	header[0] = 'R'; // RIFF/WAVE header
	header[1] = 'I';
	header[2] = 'F';
	header[3] = 'F';
	header[4] = (byte) (waveHeader->totalDataLen & 0xff);
	header[5] = (byte) ((waveHeader->totalDataLen >> 8) & 0xff);
	header[6] = (byte) ((waveHeader->totalDataLen >> 16) & 0xff);
	header[7] = (byte) ((waveHeader->totalDataLen >> 24) & 0xff);
	header[8] = 'W';
	header[9] = 'A';
	header[10] = 'V';
	header[11] = 'E';
	header[12] = 'f'; // 'fmt ' chunk
	header[13] = 'm';
	header[14] = 't';
	header[15] = ' ';
	header[16] = 16; // 4 bytes: size of 'fmt ' chunk
	header[17] = 0;
	header[18] = 0;
	header[19] = 0;
	header[20] = 1; // format = 1
	header[21] = 0;
	header[22] = (byte) waveHeader->channels;
	header[23] = 0;
	header[24] = (byte) (waveHeader->longSampleRate & 0xff);
	header[25] = (byte) ((waveHeader->longSampleRate >> 8) & 0xff);
	header[26] = (byte) ((waveHeader->longSampleRate >> 16) & 0xff);
	header[27] = (byte) ((waveHeader->longSampleRate >> 24) & 0xff);
	header[28] = (byte) (waveHeader->byteRate & 0xff);
	header[29] = (byte) ((waveHeader->byteRate >> 8) & 0xff);
	header[30] = (byte) ((waveHeader->byteRate >> 16) & 0xff);
	header[31] = (byte) ((waveHeader->byteRate >> 24) & 0xff);
	header[32] = (byte) (2 * 16 / 8); // block align
	header[33] = 0;
	header[34] = 16; // bits per sample
	header[35] = 0;
	header[36] = 'd';
	header[37] = 'a';
	header[38] = 't';
	header[39] = 'a';
	header[40] = (byte) (waveHeader->totalAudioLen & 0xff);
	header[41] = (byte) ((waveHeader->totalAudioLen >> 8) & 0xff);
	header[42] = (byte) ((waveHeader->totalAudioLen >> 16) & 0xff);
	header[43] = (byte) ((waveHeader->totalAudioLen >> 24) & 0xff);
	return header;
}


static inline void convertToWavFileFromPCMFile(const char* pcmFilePath,
		const  char* wavFilePath, int audioSampleRate, int channels) {
	long byteRate = audioSampleRate * channels;
	FILE* input_file = NULL;
	FILE* output_file = NULL;
	input_file = fopen(pcmFilePath, "rb");
	output_file = fopen(wavFilePath, "wb+");
	if (NULL != input_file && NULL != output_file) {
		fseek(input_file, 0, SEEK_END);
		long totalAudioLen = ftell(input_file);
		rewind(input_file);
 		long totalDataLen = totalAudioLen + 36;
		LOGI("totalAudioLen is %ld", totalAudioLen);
		WaveHeader* waveHeader = new WaveHeader();
		waveHeader->totalDataLen = totalDataLen + 44;
		waveHeader->channels = channels;
		waveHeader->longSampleRate = audioSampleRate;
		waveHeader->byteRate = byteRate;
		waveHeader->totalAudioLen = totalAudioLen;
		byte* header = WriteWaveFileHeader(waveHeader);
		fwrite(header, 1, 44, output_file);
		byte* buffer = new byte[1024 * 4];
		int length = -1;
		while ((length = fread(buffer, 1, 1024 * 4, input_file)) > 0) {
			fwrite(buffer, 1, length, output_file);
		}
		fclose(input_file);
		fclose(output_file);
	} else {
		LOGI("input file or output file invalid... \n");
	}
}
#endif //SONG_STUDIO_WAV_TOOLS_H_
