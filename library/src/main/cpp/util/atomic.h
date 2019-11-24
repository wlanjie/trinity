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

#endif //TRINITY_ATOMIC_H
