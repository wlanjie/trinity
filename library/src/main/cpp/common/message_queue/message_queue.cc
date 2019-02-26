//
// Created by wlanjie on 2019/2/21.
//

#include <pthread.h>
#include "message_queue.h"
#include "handler.h"

namespace trinity {

Message::Message() {
    handler = nullptr;
}

Message::Message(int what) {
    handler = nullptr;
    what_ = what;
}

Message::Message(int what, int arg1, int arg2) {
    handler = nullptr;
    what_ = what;
    arg1_ = arg1;
    arg2_ = arg2;
}

Message::Message(int what, void *obj) {
    handler = nullptr;
    what_ = what;
    obj_ = obj;
}

Message::Message(int what, int arg1, int arg2, void *obj) {
    handler = nullptr;
    what_ = what;
    arg1_ = arg1;
    arg2_ = arg2;
    obj_ = obj;
}

int Message::execute() {
    if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == what_) {
        return MESSAGE_QUEUE_LOOP_QUIT_FLAG;
    } else if (handler) {
        handler->HandleMessage(this);
        return 1;
    }
    return 0;
}

MessageQueue::MessageQueue() {
    Init();
}

MessageQueue::MessageQueue(const char *queue_name) {
    Init();
    queue_name_ = queue_name;
}

MessageQueue::~MessageQueue() {
    Flush();
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&condition_);
}

void MessageQueue::Init() {
    int init_lock_code = pthread_mutex_init(&lock_, nullptr);
    int init_condition_code = pthread_cond_init(&condition_, nullptr);
    nb_packets_ = 0;
    first_ = nullptr;
    last_ = nullptr;
    abort_request_ = false;
}

void MessageQueue::Flush() {
    MessageNode* cur_node, *next_node;
    Message* msg;
    pthread_mutex_lock(&lock_);
    for (cur_node = first_; cur_node != nullptr; cur_node = next_node) {
        next_node = cur_node->next;
        msg = cur_node->msg;
        if (nullptr != msg) {
            delete msg;
        }
        delete cur_node;
        cur_node = nullptr;
    }
    last_ = nullptr;
    first_ = nullptr;
    nb_packets_ = 0;
    pthread_mutex_unlock(&lock_);
}

int MessageQueue::EnqueueMessage(Message *msg) {
    if (abort_request_) {
        delete msg;
        return -1;
    }
    MessageNode* node = new MessageNode();
    if (node == nullptr) {
        return -1;
    }
    node->msg = msg;
    node->next = nullptr;
    pthread_mutex_lock(&lock_);
    if (last_ == nullptr) {
        first_ = node;
    } else {
        last_->next = node;
    }
    last_ = node;
    nb_packets_++;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
    return 0;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
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
            nb_packets_--;
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

int MessageQueue::Size() {
    pthread_mutex_lock(&lock_);
    int size = nb_packets_;
    pthread_mutex_unlock(&lock_);
    return size;
}

void MessageQueue::Abort() {
    pthread_mutex_lock(&lock_);
    abort_request_ = true;
    pthread_cond_signal(&condition_);
    pthread_mutex_unlock(&lock_);
}

}