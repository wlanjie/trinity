//
// Created by wlanjie on 2019/2/21.
//

#ifndef TRINITY_MESSAGE_QUEUE_H
#define TRINITY_MESSAGE_QUEUE_H

#include <pty.h>

#define MESSAGE_QUEUE_LOOP_QUIT_FLAG        19900909

namespace trinity {

class Handler;

class Message {
private:
    int what_;
    int arg1_;
    int arg2_;
    void* obj_;

public:
    Message();
    Message(int what);
    Message(int what, int arg1, int arg2);
    Message(int what, void* obj);
    Message(int what, int arg1, int arg2, void* obj);

    int execute();

    int GetWhat() {
        return what_;
    }

    int GetArg1() {
        return arg1_;
    }

    int GetArg2() {
        return arg2_;
    }

    void* GetObj() {
        return obj_;
    }

    Handler* handler;
};

typedef struct MessageNode {
    Message* msg;
    struct MessageNode* next;
    MessageNode() {
        msg = nullptr;
        next = nullptr;
    }
} MessageNode;

class MessageQueue {
private:
    MessageNode* first_;
    MessageNode* last_;
    int nb_packets_;
    bool abort_request_;
    pthread_mutex_t lock_;
    pthread_cond_t condition_;
    const char* queue_name_;

public:
    MessageQueue();
    MessageQueue(const char* queue_name);
    ~MessageQueue();

    void Init();
    void Flush();
    int EnqueueMessage(Message* msg);
    int DequeueMessage(Message** msg, bool block);
    int Size();
    void Abort();
};

}
#endif //TRINITY_MESSAGE_QUEUE_H
