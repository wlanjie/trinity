#ifndef VIDEO_COMMON_MESSAGE_QUEUE_H
#define VIDEO_COMMON_MESSAGE_QUEUE_H

#include "common_tools.h"
#include <pthread.h>

#define MESSAGE_QUEUE_LOOP_QUIT_FLAG        19900909

class Handler;

class Message {
private:
	int what;
	int arg1;
	int arg2;
	void* obj;

public:
	Message();
	Message(int what);
	Message(int what, int arg1, int arg2);
	Message(int what, void* obj);
	Message(int what, int arg1, int arg2, void* obj);
	~Message();

	int Execute();
	int GetWhat(){
		return what;
	};
	int GetArg1(){
		return arg1;
	};
	int GetArg2(){
		return arg2;
	};
	void* GetObj(){
		return obj;
	};
	Handler* handler_;
};

typedef struct MessageNode {
	Message *msg;
	struct MessageNode *next;
	MessageNode(){
		msg = NULL;
		next = NULL;
	}
} MessageNode;

class MessageQueue {
private:
	MessageNode* first_;
	MessageNode* last_;
	int packet_size_;
	bool abort_request_;
	pthread_mutex_t lock_;
	pthread_cond_t condition_;
	const char* queue_name_;


public:
	MessageQueue();
	MessageQueue(const char* queueNameParam);
	~MessageQueue();

	void Init();
	void Flush();
	int EnqueueMessage(Message *msg);
	int DequeueMessage(Message **msg, bool block);
	int Size();
	void Abort();

};

#endif // VIDEO_COMMON_MESSAGE_QUEUE_H
