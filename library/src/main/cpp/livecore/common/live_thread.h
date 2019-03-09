#ifndef LIVE_THREAD_H
#define LIVE_THREAD_H

#include <pthread.h>
#include "../platform_dependent/platform_4_live_common.h"

class LiveThread {
public:
	LiveThread();
	~LiveThread();

	void Start();
	void StartAsync();
	int Wait();
	void WaitOnNotify();
	void Notify();
	virtual void Stop();

protected:
	bool running_;
	virtual void HandleRun(void *ptr);

protected:
	pthread_t thread_;
	pthread_mutex_t lock_;
	pthread_cond_t condition_;
	static void* StartThread(void *ptr);
};

#endif //LIVE_THREAD_H
