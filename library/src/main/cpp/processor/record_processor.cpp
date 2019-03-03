#include "record_processor.h"

#define LOG_TAG "SongStudio RecordProcessor"

RecordProcessor::RecordProcessor() {
	audioSamples = NULL;
	audioEncoder = NULL;
	isRecordingFlag = false;
	dataAccumulateTimeMills = 0;
}

RecordProcessor::~RecordProcessor() {
}

void RecordProcessor::initAudioBufferSize(int sampleRate, int audioBufferSizeParam) {
	audioSamplesCursor = 0;
	audioBufferSize = audioBufferSizeParam;
	audioSampleRate = sampleRate;
	audioSamples = new short[audioBufferSize];
	packetPool = LiveCommonPacketPool::GetInstance();
	corrector = new RecordCorrector();
	audioBufferTimeMills = (float)audioBufferSize * 1000.0f / (float)audioSampleRate;
}

void RecordProcessor::flushAudioBufferToQueue() {
	if (audioSamplesCursor > 0) {
		if(NULL == audioEncoder){
			audioEncoder = new AudioProcessEncoderAdapter();
			int audioChannels = 2;
			int audioBitRate = 64 * 1024;
			const char* audioCodecName = "libfdk_aac";
			audioEncoder->init(packetPool, audioSampleRate, audioChannels, audioBitRate, audioCodecName);
		}
		short* packetBuffer = new short[audioSamplesCursor];
		if (NULL == packetBuffer) {
			return;
		}
		memcpy(packetBuffer, audioSamples, audioSamplesCursor * sizeof(short));
		LiveAudioPacket * audioPacket = new LiveAudioPacket();
		audioPacket->buffer = packetBuffer;
		audioPacket->size = audioSamplesCursor;
		packetPool->pushAudioPacketToQueue(audioPacket);
		audioSamplesCursor = 0;
		dataAccumulateTimeMills+=audioBufferTimeMills;
//		LOGI("audioSamplesTimeMills is %.6f SampleCnt Cal timeMills %llu", audioSamplesTimeMills, dataAccumulateTimeMills);
		int correctDurationInTimeMills = 0;
		if(corrector->detectNeedCorrect(dataAccumulateTimeMills, audioSamplesTimeMills, &correctDurationInTimeMills)){
			//检测到有问题了, 需要进行修复
			this->correctRecordBuffer(correctDurationInTimeMills);
		}
	}
}

int RecordProcessor::correctRecordBuffer(int correctTimeMills) {
	LOGI("RecordProcessor::correctRecordBuffer... correctTimeMills is %d", correctTimeMills);
	int correctBufferSize = ((float)correctTimeMills / 1000.0f) * audioSampleRate;
	LiveAudioPacket * audioPacket = getSilentDataPacket(correctBufferSize);
	packetPool->pushAudioPacketToQueue(audioPacket);
	LiveAudioPacket * accompanyPacket = getSilentDataPacket(correctBufferSize);
	packetPool->pushAccompanyPacketToQueue(accompanyPacket);
	dataAccumulateTimeMills+=correctTimeMills;
}

LiveAudioPacket* RecordProcessor::getSilentDataPacket(int audioBufferSize) {
	LiveAudioPacket * audioPacket = new LiveAudioPacket();
	audioPacket->buffer = new short[audioBufferSize];
	memset(audioPacket->buffer, 0, audioBufferSize * sizeof(short));
	audioPacket->size = audioBufferSize;
	return audioPacket;
}

int RecordProcessor::pushAudioBufferToQueue(short* samples, int size) {
	if (size <= 0) {
		return size;
	}
	if(!isRecordingFlag){
		isRecordingFlag = true;
		startTimeMills = currentTimeMills();
	}
	int samplesCursor = 0;
	int samplesCnt = size;
	while (samplesCnt > 0) {
		if ((audioSamplesCursor + samplesCnt) < audioBufferSize) {
			this->cpyToAudioSamples(samples + samplesCursor, samplesCnt);
			audioSamplesCursor += samplesCnt;
			samplesCursor += samplesCnt;
			samplesCnt = 0;
		} else {
			int subFullSize = audioBufferSize - audioSamplesCursor;
			this->cpyToAudioSamples(samples + samplesCursor, subFullSize);
			audioSamplesCursor += subFullSize;
			samplesCursor += subFullSize;
			samplesCnt -= subFullSize;
			flushAudioBufferToQueue();
		}
	}
	return size;
}

void RecordProcessor::cpyToAudioSamples(short* sourceBuffer, int cpyLength) {
	if(0 == audioSamplesCursor){
		audioSamplesTimeMills = currentTimeMills() - startTimeMills;
	}
	memcpy(audioSamples + audioSamplesCursor, sourceBuffer, cpyLength * sizeof(short));
}

void RecordProcessor::destroy() {
	if(NULL != audioSamples){
		delete[] audioSamples;
		audioSamples = NULL;
	}
	if(NULL != audioEncoder){
		audioEncoder->destroy();
		delete audioEncoder;
		audioEncoder = NULL;
	}
	if(NULL != corrector){
		delete corrector;
		corrector = NULL;
	}
}
