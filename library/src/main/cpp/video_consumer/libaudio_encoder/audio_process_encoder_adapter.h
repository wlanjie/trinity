#ifndef AUDIO_PROCESS_ENCODER_ADAPTER_H
#define AUDIO_PROCESS_ENCODER_ADAPTER_H

#include "encoder/audio_encoder_adapter.h"
#include "libmusic_merger/music_merger.h"
#include "libvideo_consumer/live_common_packet_pool.h"

class AudioProcessEncoderAdapter: public AudioEncoderAdapter {
public:
	AudioProcessEncoderAdapter();
    virtual ~AudioProcessEncoderAdapter();

    void init(LivePacketPool* pcmPacketPool, int audioSampleRate, int audioChannels,
    		int audioBitRate, const char* audio_codec_name);

    virtual void destroy();
protected:
	LiveCommonPacketPool* accompanyPacketPool;
	MusicMerger* musicMerger;

    virtual void discardAudioPacket();
    virtual int processAudio();

};
#endif // AUDIO_PROCESS_ENCODER_ADAPTER_H
