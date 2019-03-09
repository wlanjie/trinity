#include "./message_queue.h"
#include "./handler.h"

#define LOG_TAG "MessageQueue"

/******************* MessageQueue class *******************/
MessageQueue::MessageQueue() {
	init();
}

MessageQueue::MessageQueue(const char* queueNameParam) {
	init();
	queueName = queueNameParam;
}

void MessageQueue::init() {
	int initLockCode = pthread_mutex_init(&mLock, NULL);
	int initConditionCode = pthread_cond_init(&mCondition, NULL);
	mNbPackets = 0;
	mFirst = NULL;
	mLast = NULL;
	mAbortRequest = false;
}

MessageQueue::~MessageQueue() {
	LOGI("%s ~PacketQueue ....", queueName);
	flush();
	pthread_mutex_destroy(&mLock);
	pthread_cond_destroy(&mCondition);
}

int MessageQueue::size() {
	pthread_mutex_lock(&mLock);
	int size = mNbPackets;
	pthread_mutex_unlock(&mLock);
	return size;
}

void MessageQueue::flush() {
	LOGI("\n %s flush .... and this time the queue_ size is %d \n", queueName, size());
	MessageNode *curNode, *nextNode;
	Message *msg;
	pthread_mutex_lock(&mLock);
	for (curNode = mFirst; curNode != NULL; curNode = nextNode) {
		nextNode = curNode->next;
		msg = curNode->msg;
		if (NULL != msg) {
			delete msg;
		}
		delete curNode;
		curNode = NULL;
	}
	mLast = NULL;
	mFirst = NULL;
	mNbPackets = 0;
	pthread_mutex_unlock(&mLock);
}

int MessageQueue::enqueueMessage(Message* msg) {
	if (mAbortRequest) {
		delete msg;
		return -1;
	}
	MessageNode *node = new MessageNode();
	if (!node)
		return -1;
	node->msg = msg;
	node->next = NULL;
	int getLockCode = pthread_mutex_lock(&mLock);
	if (mLast == NULL) {
		mFirst = node;
	} else {
		mLast->next = node;
	}
	mLast = node;
	mNbPackets++;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
	return 0;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int MessageQueue::dequeueMessage(Message **msg, bool block) {
	MessageNode *node;
	int ret;
	int getLockCode = pthread_mutex_lock(&mLock);
	for (;;) {
		if (mAbortRequest) {
			ret = -1;
			break;
		}
		node = mFirst;
		if (node) {
			mFirst = node->next;
			if (!mFirst)
				mLast = NULL;
			mNbPackets--;
			*msg = node->msg;
			delete node;
			node = NULL;
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			pthread_cond_wait(&mCondition, &mLock);
		}
	}
	pthread_mutex_unlock(&mLock);
	return ret;
}

void MessageQueue::abort() {
	pthread_mutex_lock(&mLock);
	mAbortRequest = true;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}


/******************* Message class *******************/
Message::Message() {
	handler = NULL;
}

Message::Message(int what){
	handler = NULL;
	this->what = what;
}
Message::Message(int what, int arg1, int arg2) {
	handler = NULL;
	this->what = what;
	this->arg1 = arg1;
	this->arg2 = arg2;
}
Message::Message(int what, void* obj) {
	handler = NULL;
	this->what = what;
	this->obj = obj;
}
Message::Message(int what, int arg1, int arg2, void* obj) {
	handler = NULL;
	this->what = what;
	this->arg1 = arg1;
	this->arg2 = arg2;
	this->obj = obj;
}
Message::~Message() {
}

int Message::execute(){
	if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == what) {
		return MESSAGE_QUEUE_LOOP_QUIT_FLAG;
	} else if (handler) {
		handler->handleMessage(this);
		return 1;
	}
	return 0;
};
