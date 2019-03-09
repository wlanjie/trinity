#include "packet_pool.h"

#define LOG_TAG "LivePacketPool"

LivePacketPool::LivePacketPool() {
    audio_packet_queue_ = NULL;
    recording_video_packet_queue_ = NULL;
    buffer_ = NULL;
    pthread_rwlock_init(&rw_lock_, NULL);
}

LivePacketPool::~LivePacketPool() {
    pthread_rwlock_destroy(&rw_lock_);
}

LivePacketPool* LivePacketPool::instance_ = new LivePacketPool();
LivePacketPool* LivePacketPool::GetInstance() {
    return instance_;
}

void LivePacketPool::InitAudioPacketQueue(int audioSampleRate) {
    const char* name = "audioPacket queue_";
    audio_packet_queue_ = new LiveAudioPacketQueue(name);
    // TODO 是否使用
    this->audio_sample_rate_ = audioSampleRate;
#ifdef __ANDROID__
    this->channels_ = INPUT_CHANNEL_4_ANDROID;
#elif defined(__APPLE__)	// IOS or OSX
    this->channels_ = INPUT_CHANNEL_4_IOS;
#endif
    buffer_size_ = audioSampleRate * channels_ * AUDIO_PACKET_DURATION_IN_SECS;
    buffer_ = new short[buffer_size_];
    buffer_cursor_ = 0;
}

void LivePacketPool::AbortAudioPacketQueue() {
    if(NULL != audio_packet_queue_){
        audio_packet_queue_->Abort();
    }
}
void LivePacketPool::DestroyAudioPacketQueue() {
    if(NULL != audio_packet_queue_){
        delete audio_packet_queue_;
        audio_packet_queue_ = NULL;
    }
    if(buffer_){
        delete[] buffer_;
        buffer_ = NULL;
    }
}

int LivePacketPool::GetAudioPacket(LiveAudioPacket **audioPacket, bool block) {
    int result = -1;
    if(NULL != audio_packet_queue_){
        result = audio_packet_queue_->Get(audioPacket, block);
    }
    return result;
}

int LivePacketPool::GetAudioPacketQueueSize() {
    return audio_packet_queue_->Size();
}

bool LivePacketPool::DiscardAudioPacket() {
    bool ret = false;
    LiveAudioPacket *tempAudioPacket = NULL;
    int resultCode = audio_packet_queue_->Get(&tempAudioPacket, true);
    if (resultCode > 0) {
        delete tempAudioPacket;
        tempAudioPacket = NULL;
        pthread_rwlock_wrlock(&rw_lock_);
        total_discard_video_packet_duration_ -= (AUDIO_PACKET_DURATION_IN_SECS * 1000.0f);
        pthread_rwlock_unlock(&rw_lock_);
        ret = true;
    }
//    LOGI("discard %d ms Audio Data And this time total_discard_video_packet_duration_ is %d GetAudioPacketQueueSize is %d", (int)(AUDIO_PACKET_DURATION_IN_SECS * 1000.0f), total_discard_video_packet_duration_, GetAudioPacketQueueSize());
    return ret;
}

bool LivePacketPool::DetectDiscardAudioPacket() {
    bool ret = false;
    pthread_rwlock_wrlock(&rw_lock_);
    ret = total_discard_video_packet_duration_ >= (AUDIO_PACKET_DURATION_IN_SECS * 1000.0f);
    pthread_rwlock_unlock(&rw_lock_);
    return ret;
}

void LivePacketPool::PushAudioPacketToQueue(LiveAudioPacket *audioPacket) {
    if(NULL != audio_packet_queue_){
        int audioPacketBufferCursor = 0;
//        float position_ = audioPacket->position_;
        while(audioPacket->size > 0){
            int audioBufferLength = buffer_size_ - buffer_cursor_;
            int length = MIN(audioBufferLength, audioPacket->size);
            memcpy(buffer_ + buffer_cursor_, audioPacket->buffer + audioPacketBufferCursor, length * sizeof(short));
            audioPacket->size -= length;
            buffer_cursor_ += length;
            audioPacketBufferCursor += length;
            if(buffer_cursor_ == buffer_size_){
                LiveAudioPacket* targetAudioPacket = new LiveAudioPacket();
                targetAudioPacket->size = buffer_size_;
                short * audioBuffer = new short[buffer_size_];
                memcpy(audioBuffer, buffer_, buffer_size_ * sizeof(short));
                targetAudioPacket->buffer = audioBuffer;
//                int consumeBufferCnt = audioPacketBufferCursor - length;
//                float consumeBufferDuration = ((float)consumeBufferCnt / (float)(this->channels_ * sample_rate_)) * 1000.0f;
//                targetAudioPacket->position_ = position_ + consumeBufferDuration;
//        			LOGI("AudioPacket Split : targetAudioPacket->position_ is %.6f", targetAudioPacket->position_);
                audio_packet_queue_->Put(targetAudioPacket);
                buffer_cursor_ = 0;
            }
        }
        delete audioPacket;
    }
}
/**************************end audio packet queue process**********************************************/

/**************************start decoder original song packet queue process*******************************************/

void LivePacketPool::InitRecordingVideoPacketQueue() {
    if(NULL == recording_video_packet_queue_){
        const char* name = "recording video yuv frame_ packet_ queue_";
        recording_video_packet_queue_ = new LiveVideoPacketQueue(name);
        total_discard_video_packet_duration_ = 0;
        temp_video_packet_ = NULL;
        temp_video_packet_ref_count_ = 0;
    }
}

void LivePacketPool::AbortRecordingVideoPacketQueue() {
    if (NULL != recording_video_packet_queue_) {
        recording_video_packet_queue_->Abort();
    }
}

void LivePacketPool::DestroyRecordingVideoPacketQueue() {
    if (NULL != recording_video_packet_queue_) {
        delete recording_video_packet_queue_;
        recording_video_packet_queue_ = NULL;
        if(temp_video_packet_ref_count_ > 0){
            delete temp_video_packet_;
            temp_video_packet_ = NULL;
        }
    }
}

int LivePacketPool::GetRecordingVideoPacket(LiveVideoPacket **videoPacket,
                                            bool block) {
    int result = -1;
    if (NULL != recording_video_packet_queue_) {
        result = recording_video_packet_queue_->Get(videoPacket, block);
    }
    return result;
}

bool LivePacketPool::DetectDiscardVideoPacket(){
    return recording_video_packet_queue_->Size() > VIDEO_PACKET_QUEUE_THRRESHOLD;
}

bool LivePacketPool::PushRecordingVideoPacketToQueue(LiveVideoPacket *videoPacket) {
    bool dropFrame = false;
    if (NULL != recording_video_packet_queue_) {
        while (DetectDiscardVideoPacket()) {
            dropFrame = true;
            int discardVideoFrameCnt = 0;
            int discardVideoFrameDuration = recording_video_packet_queue_->DiscardGOP(&discardVideoFrameCnt);
            if(discardVideoFrameDuration < 0){
                break;
            }
            this->RecordDropVideoFrame(discardVideoFrameDuration);
            //            LOGI("discard a GOP Video Packet And totalDiscardVideoPacketSize is %d", totalDiscardVideoPacketSize);
        }
        //为了计算当前帧的Duration, 所以延迟一帧放入Queue中
        if(NULL != temp_video_packet_){
            int packetDuration = videoPacket->timeMills - temp_video_packet_->timeMills;
            temp_video_packet_->duration = packetDuration;
            recording_video_packet_queue_->Put(temp_video_packet_);
            temp_video_packet_ref_count_ = 0;
        }
        temp_video_packet_ = videoPacket;
        temp_video_packet_ref_count_ = 1;
    }
    return dropFrame;
}

void LivePacketPool::RecordDropVideoFrame(int discardVideoPacketDuration){
    pthread_rwlock_wrlock(&rw_lock_);
    total_discard_video_packet_duration_+=discardVideoPacketDuration;
    pthread_rwlock_unlock(&rw_lock_);
}

int LivePacketPool::GetRecordingVideoPacketQueueSize() {
    if (NULL != recording_video_packet_queue_) {
        return recording_video_packet_queue_->Size();
    }
    return 0;
}

void LivePacketPool::ClearRecordingVideoPacketToQueue() {
    if (NULL != recording_video_packet_queue_) {
        return recording_video_packet_queue_->Flush();
    }
}
/**************************end decoder original song packet queue process*******************************************/
