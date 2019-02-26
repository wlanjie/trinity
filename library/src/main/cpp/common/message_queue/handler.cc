//
// Created by wlanjie on 2019/2/21.
//

#include "handler.h"

namespace trinity {

Handler::Handler(MessageQueue *queue) {
    queue_ = queue;
}

Handler::~Handler() {

}

int Handler::PostMessage(Message *msg) {
    msg->handler = this;
    return queue_->EnqueueMessage(msg);
}

int Handler::GetQueueSize() {
    return queue_->Size();
}

}