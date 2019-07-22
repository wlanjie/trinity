//
// Created by wlanjie on 2019/4/13.
//

#include "handler.h"

namespace trinity {

Handler::Handler(MessageQueue* queue) {
    this->mQueue = queue;
}

Handler::~Handler() {
}

int Handler::PostMessage(Message *msg){
    msg->handler_ = this;
    return mQueue->EnqueueMessage(msg);
}

int Handler::GetQueueSize() {
    return mQueue->Size();
}

}