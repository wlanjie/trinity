#ifndef SONGSTUDIO_RECORDER_CORRECTOR_H
#define SONGSTUDIO_RECORDER_CORRECTOR_H
#include "CommonTools.h"

#define MIN_DIFF_TIME_MILLS		50
#define MAX_DIFF_TIME_MILLS		150

class RecordCorrector {
public:
	RecordCorrector();
	~RecordCorrector();

	bool detectNeedCorrect(int64_t dataPresentTimeMills, int64_t recordingTimeMills, int* correctTimeMills);
};

#endif //SONGSTUDIO_RECORDER_CORRECTOR_H
