#ifndef SONGSTUDIO_RECORDER_H
#define SONGSTUDIO_RECORDER_H
#include "common_tools.h"
#include "../../video_consumer/libaudio_encoder/audio_process_encoder_adapter.h"
#include "libvideo_consumer/live_common_packet_pool.h"
#include "record_corrector.h"

class RecordProcessor {
private:
	int audio_sample_rate_;
	short* audio_samples_;
	float audio_samples_time_mills_;
	int64_t data_accumulate_time_mills_;
	int audio_samples_cursor_;
	int audio_buffer_size_;
	int audio_buffer_time_mills_;
	LiveCommonPacketPool* packet_pool_;
	AudioProcessEncoderAdapter* audio_encoder_;
	RecordCorrector* corrector_;
	bool recording_flag_;
	long long start_time_mills_;
	void CpyToAudioSamples(short *sourceBuffer, int cpyLength);
	LiveAudioPacket* GetSilentDataPacket(int audioBufferSize);
	int CorrectRecordBuffer(int correctTimeMills);
public:
	RecordProcessor();
	~RecordProcessor();

	//关于录音器录制出来的人声的处理
	void InitAudioBufferSize(int sampleRate, int audioBufferSizeParam);
	int PushAudioBufferToQueue(short *samplesParam, int size);
	void FlushAudioBufferToQueue();
	void Destroy();
};

#endif //SONGSTUDIO_RECORDER_H
