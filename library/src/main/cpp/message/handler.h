//
// Created by wlanjie on 2019/4/13.
//

#ifndef TRINITY_HANDLER_H
#define TRINITY_HANDLER_H

#include "handler.h"
#include "message_queue.h"

namespace trinity {

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

}

#endif //TRINITY_HANDLER_H
