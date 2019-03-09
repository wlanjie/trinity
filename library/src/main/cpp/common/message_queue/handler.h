#ifndef VIDEO_COMMON_HANDLER_H
#define VIDEO_COMMON_HANDLER_H

#include "common_tools.h"
#include "./message_queue.h"

class Handler {
private:
	MessageQueue* mQueue;

public:
	Handler(MessageQueue* mQueue);
	~Handler();

	int PostMessage(Message *msg);
	int GetQueueSize();
	virtual void HandleMessage(Message *msg){};
};

#endif // VIDEO_COMMON_HANDLER_H
