#include "live_thread.h"

#define LOG_TAG "LiveThread"

LiveThread::LiveThread() {
	pthread_mutex_init(&lock_, NULL);
	pthread_cond_init(&condition_, NULL);
}

LiveThread::~LiveThread() {
}

void LiveThread::Start() {
	HandleRun(NULL);
}

void LiveThread::StartAsync() {
	pthread_create(&thread_, NULL, StartThread, this);
}

int LiveThread::Wait() {
	if (!running_) {
		LOGI("running_ is false so return 0");
		return 0;
	}
	void* status;
    int ret = pthread_join(thread_,&status);
	LOGI("pthread_join thread return result is %d ", status);
	return (int)ret;
}

void LiveThread::Stop() {
}

void* LiveThread::StartThread(void *ptr) {
	LOGI("starting thread");
	LiveThread* thread = (LiveThread *) ptr;
	thread->running_ = true;
	thread->HandleRun(ptr);
	thread->running_ = false;
	return NULL;
}

void LiveThread::WaitOnNotify() {
	pthread_mutex_lock(&lock_);
	pthread_cond_wait(&condition_, &lock_);
	pthread_mutex_unlock(&lock_);
}

void LiveThread::Notify() {
	pthread_mutex_lock(&lock_);
	pthread_cond_signal(&condition_);
	pthread_mutex_unlock(&lock_);
}

void LiveThread::HandleRun(void *ptr) {
}
