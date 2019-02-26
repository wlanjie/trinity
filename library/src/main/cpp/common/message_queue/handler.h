//
// Created by wlanjie on 2019/2/21.
//

#ifndef TRINITY_HANDLER_H
#define TRINITY_HANDLER_H

#include "message_queue.h"

namespace trinity {

class Handler {
public:
    Handler(MessageQueue* queue);
    ~Handler();

    int PostMessage(Message* msg);
    int GetQueueSize();
    virtual void HandleMessage(Message* msg) {};

private:
    MessageQueue* queue_;
};

}
#endif //TRINITY_HANDLER_H
