#ifndef VIDEO_COMMON_MESSAGE_QUEUE_H
#define VIDEO_COMMON_MESSAGE_QUEUE_H

#include "CommonTools.h"
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

	int execute();
	int getWhat(){
		return what;
	};
	int getArg1(){
		return arg1;
	};
	int getArg2(){
		return arg2;
	};
	void* getObj(){
		return obj;
	};
	Handler* handler;
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
	MessageNode* mFirst;
	MessageNode* mLast;
	int mNbPackets;
	bool mAbortRequest;
	pthread_mutex_t mLock;
	pthread_cond_t mCondition;
	const char* queueName;


public:
	MessageQueue();
	MessageQueue(const char* queueNameParam);
	~MessageQueue();

	void init();
	void flush();
	int enqueueMessage(Message* msg);
	int dequeueMessage(Message **msg, bool block);
	int size();
	void abort();

};

#endif // VIDEO_COMMON_MESSAGE_QUEUE_H
