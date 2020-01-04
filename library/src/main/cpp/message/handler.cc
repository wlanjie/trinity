/*
 * Copyright (C) 2019 Trinity. All rights reserved.
 * Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//
// Created by wlanjie on 2019/4/13.
//

#include "handler.h"

namespace trinity {

Handler::Handler() : queue_(nullptr) {
}

Handler::Handler(MessageQueue* queue) {
    this->queue_ = queue;
}

Handler::~Handler() = default;

void Handler::InitMessageQueue(MessageQueue *queue) {
    this->queue_ = queue;
}

int Handler::PostMessage(Message *msg) {
    msg->handler_ = this;
    if (nullptr == queue_) {
        return 0;
    }
    return queue_->EnqueueMessage(msg);
}

int Handler::GetQueueSize() {
    if (nullptr == queue_) {
        return 0;
    }
    return queue_->Size();
}

}  // namespace trinity
