#ifndef SONG_STUDIO_THREAD_H
#define SONG_STUDIO_THREAD_H

#include <pthread.h>

class Thread {
public:
	Thread();
	~Thread();

	void start();
	void startAsync();
	int wait();

	void waitOnNotify();
	void notify();
	virtual void stop();

protected:
	bool mRunning;

	virtual void handleRun(void* ptr);

protected:
	pthread_t mThread;
	pthread_mutex_t mLock;
	pthread_cond_t mCondition;

	static void* startThread(void* ptr);
};

#endif //SONG_STUDIO_THREAD_H
