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
// Created by wlanjie on 2019-12-11.
//

#ifndef TRINITY_ATOMIC_H
#define TRINITY_ATOMIC_H

int android_atomic_add(int increment, int* ptr) {
    *ptr = *ptr + increment;
    return *ptr - increment;
}

int android_atomic_inc(int* addr) {
    return android_atomic_add(1, addr);
}

int android_atomic_dec(int * addr) {
    return android_atomic_add(-1, addr);
}

int android_atomic_or(int value, int* ptr) {
    int old = *ptr;
    *ptr = *ptr | value;
    return old;
}

int android_atomic_cmpxchg(int oldvalue, int newvalue, int* addr) {
    int result;
    if (*addr == oldvalue) {
        *addr = newvalue;
        result = 0;
    } else {
        result = 1;
    }
    return result;
}

#endif  // TRINITY_ATOMIC_H
