/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
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
// Created by wlanjie on 2020/4/23.
//

#ifndef TRINITY_BUFFER_POOL_H
#define TRINITY_BUFFER_POOL_H

#include <set>
#include <vector>
#include <mutex>
#include "android_xlog.h"

namespace trinity {

class BufferPool {
 public:
    explicit BufferPool(int buffer_size) : buffer_size_(buffer_size) {};
    virtual ~BufferPool() {
        LOGI("enter: %s size: %d", __func__, buffers_.size());
        for (auto buffer : buffers_) {
            free(buffer);
        }
        buffers_.clear();
        LOGI("leave: %s", __func__);
    }

    // int* p = buffer_->GetBuffer<int>();
    template<typename T>
    T* GetBuffer() {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!buffers_.empty()) {
            auto buffer = buffers_.back();
            buffers_.pop_back();
            return reinterpret_cast<T*>(buffer);
        } else {
            auto buffer = malloc(static_cast<size_t>(buffer_size_));
            return reinterpret_cast<T*>(buffer);
        }
    }

    template<typename T>
    void ReleaseBuffer(T* buffer) {
        if (nullptr == buffer) {
            return;
        }
        std::lock_guard<std::mutex> guard(mutex_);
        buffers_.push_back(buffer);
    }

 private:
    int buffer_size_;
    std::mutex mutex_;
    std::vector<void*> buffers_;
};

}

#endif  // TRINITY_BUFFER_POOL_H
