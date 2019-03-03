#include "./handler.h"

#define LOG_TAG "Handler"

/******************* Handler class *******************/
Handler::Handler(MessageQueue* queue) {
	this->mQueue = queue;
}

Handler::~Handler() {
}

int Handler::postMessage(Message* msg){
	msg->handler = this;
//	LOGI("enqueue msg what is %d", msg->getWhat());
	return mQueue->enqueueMessage(msg);
}

int Handler::getQueueSize() {
	return mQueue->size();
}