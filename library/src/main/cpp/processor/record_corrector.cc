#include "record_corrector.h"

#define LOG_TAG "RecordCorrector"

RecordCorrector::RecordCorrector() {
}

RecordCorrector::~RecordCorrector() {
}

bool RecordCorrector::detectNeedCorrect(int64_t dataPresentTimeMills, int64_t recordingTimeMills, int* correctTimeMills) {
	bool ret = false;
	(*correctTimeMills) = 0;
	if(dataPresentTimeMills <= (recordingTimeMills - MAX_DIFF_TIME_MILLS)){
		ret = true;
		(*correctTimeMills) = MAX_DIFF_TIME_MILLS - MIN_DIFF_TIME_MILLS;
	}
	return ret;
}
