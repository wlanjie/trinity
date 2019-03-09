#include "./handler.h"

#define LOG_TAG "Handler"

/******************* Handler class *******************/
Handler::Handler(MessageQueue* queue) {
	this->mQueue = queue;
}

Handler::~Handler() {
}

int Handler::PostMessage(Message *msg){
	msg->handler_ = this;
//	LOGI("enqueue msg what is %d", msg->GetWhat());
	return mQueue->EnqueueMessage(msg);
}

int Handler::GetQueueSize() {
	return mQueue->Size();
}