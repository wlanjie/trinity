#include "./message_queue.h"
#include "./handler.h"

#define LOG_TAG "MessageQueue"

/******************* MessageQueue class *******************/
MessageQueue::MessageQueue() {
	Init();
}

MessageQueue::MessageQueue(const char* queueNameParam) {
	Init();
	queue_name_ = queueNameParam;
}

void MessageQueue::Init() {
	int initLockCode = pthread_mutex_init(&lock_, NULL);
	int initConditionCode = pthread_cond_init(&condition_, NULL);
	packet_size_ = 0;
	first_ = NULL;
	last_ = NULL;
	abort_request_ = false;
}

MessageQueue::~MessageQueue() {
	LOGI("%s ~PacketQueue ....", queue_name_);
	Flush();
	pthread_mutex_destroy(&lock_);
	pthread_cond_destroy(&condition_);
}

int MessageQueue::Size() {
	pthread_mutex_lock(&lock_);
	int size = packet_size_;
	pthread_mutex_unlock(&lock_);
	return size;
}

void MessageQueue::Flush() {
	LOGI("\n %s Flush .... and this time the queue_ Size is %d \n", queue_name_, Size());
	MessageNode *curNode, *nextNode;
	Message *msg;
	pthread_mutex_lock(&lock_);
	for (curNode = first_; curNode != NULL; curNode = nextNode) {
		nextNode = curNode->next;
		msg = curNode->msg;
		if (NULL != msg) {
			delete msg;
		}
		delete curNode;
		curNode = NULL;
	}
	last_ = NULL;
	first_ = NULL;
	packet_size_ = 0;
	pthread_mutex_unlock(&lock_);
}

int MessageQueue::EnqueueMessage(Message *msg) {
	if (abort_request_) {
		delete msg;
		return -1;
	}
	MessageNode *node = new MessageNode();
	if (!node)
		return -1;
	node->msg = msg;
	node->next = NULL;
	int getLockCode = pthread_mutex_lock(&lock_);
	if (last_ == NULL) {
		first_ = node;
	} else {
		last_->next = node;
	}
	last_ = node;
	packet_size_++;
	pthread_cond_signal(&condition_);
	pthread_mutex_unlock(&lock_);
	return 0;
}

/* return < 0 if aborted, 0 if no packet_ and > 0 if packet_.  */
int MessageQueue::DequeueMessage(Message **msg, bool block) {
	MessageNode *node;
	int ret;
	int getLockCode = pthread_mutex_lock(&lock_);
	for (;;) {
		if (abort_request_) {
			ret = -1;
			break;
		}
		node = first_;
		if (node) {
			first_ = node->next;
			if (!first_)
				last_ = NULL;
			packet_size_--;
			*msg = node->msg;
			delete node;
			node = NULL;
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			pthread_cond_wait(&condition_, &lock_);
		}
	}
	pthread_mutex_unlock(&lock_);
	return ret;
}

void MessageQueue::Abort() {
	pthread_mutex_lock(&lock_);
	abort_request_ = true;
	pthread_cond_signal(&condition_);
	pthread_mutex_unlock(&lock_);
}


/******************* Message class *******************/
Message::Message() {
	handler_ = NULL;
}

Message::Message(int what){
	handler_ = NULL;
	this->what = what;
}
Message::Message(int what, int arg1, int arg2) {
	handler_ = NULL;
	this->what = what;
	this->arg1 = arg1;
	this->arg2 = arg2;
}
Message::Message(int what, void* obj) {
	handler_ = NULL;
	this->what = what;
	this->obj = obj;
}
Message::Message(int what, int arg1, int arg2, void* obj) {
	handler_ = NULL;
	this->what = what;
	this->arg1 = arg1;
	this->arg2 = arg2;
	this->obj = obj;
}
Message::~Message() {
}

int Message::Execute(){
	if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == what) {
		return MESSAGE_QUEUE_LOOP_QUIT_FLAG;
	} else if (handler_) {
        handler_->HandleMessage(this);
		return 1;
	}
	return 0;
};
