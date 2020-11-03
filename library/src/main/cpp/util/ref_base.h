//
// Created by wlanjie on 2019-12-11.
//

#ifndef TRINITY_REF_BASE_H
#define TRINITY_REF_BASE_H

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include "atomic.h"

#ifndef TEXTOUTPUT_H
#define TEXTOUTPUT_H

#include <iostream>

using namespace std;
#define TextOutput ostream
#endif

namespace trinity {

template<typename T>
class wp;

// ---------------------------------------------------------------------------

#define COMPARE_WEAK(_op_)                                      \
inline bool operator _op_ (const sp<T>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}                                                               \
inline bool operator _op_ (const T* o) const {                  \
    return m_ptr _op_ o;                                        \
}                                                               \
template<typename U>                                            \
inline bool operator _op_ (const sp<U>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}                                                               \
template<typename U>                                            \
inline bool operator _op_ (const U* o) const {                  \
    return m_ptr _op_ o;                                        \
}

#define COMPARE(_op_)                                           \
COMPARE_WEAK(_op_)                                              \
inline bool operator _op_ (const wp<T>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}                                                               \
template<typename U>                                            \
inline bool operator _op_ (const wp<U>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}

// ---------------------------------------------------------------------------

class RefBase {
public:
    void incStrong(const void *id) const;

    void decStrong(const void *id) const;

    void forceIncStrong(const void *id) const;

    //! DEBUGGING ONLY: Get current strong ref count.
    int getStrongCount() const;

    class weakref_type {
    public:
        RefBase *refBase() const;

        void incWeak(const void *id);

        void decWeak(const void *id);

        bool attemptIncStrong(const void *id);

        //! This is only safe if you have set OBJECT_LIFETIME_FOREVER.
        bool attemptIncWeak(const void *id);

        //! DEBUGGING ONLY: Get current weak ref count.
        int getWeakCount() const;

        //! DEBUGGING ONLY: Print references held on object.
        void printRefs() const;

        //! DEBUGGING ONLY: Enable tracking for this object.
        // enable -- enable/disable tracking
        // retain -- when tracking is enable, if true, then we save a stack trace
        //           for each reference and dereference; when retain == false, we
        //           match up references and dereferences and keep only the
        //           outstanding ones.

        void trackMe(bool enable, bool retain) const;
    };

    weakref_type *createWeak(const void *id) const;

    weakref_type *getWeakRefs() const;

    //! DEBUGGING ONLY: Print references held on object.
    inline void printRefs() const { getWeakRefs()->printRefs(); }

    //! DEBUGGING ONLY: Enable tracking of object.
    inline void trackMe(bool enable, bool retain) {
        getWeakRefs()->trackMe(enable, retain);
    }

protected:
    RefBase();

    virtual                 ~RefBase();

    //! Flags for extendObjectLifetime()
    enum {
        OBJECT_LIFETIME_WEAK = 0x0001,
        OBJECT_LIFETIME_FOREVER = 0x0003
    };

    void extendObjectLifetime(int mode);

    //! Flags for onIncStrongAttempted()
    enum {
        FIRST_INC_STRONG = 0x0001
    };

    virtual void onFirstRef();

    virtual void onLastStrongRef(const void *id);

    virtual bool onIncStrongAttempted(unsigned int flags, const void *id);

    virtual void onLastWeakRef(const void *id);

private:
    friend class weakref_type;

    class weakref_impl;

    RefBase(const RefBase &o);

    RefBase &operator=(const RefBase &o);

    weakref_impl *const mRefs;
};

// ---------------------------------------------------------------------------

template<class T>
class LightRefBase {
public:
    inline LightRefBase() : mCount(0) {}

    inline void incStrong(const void *id) const {
        android_atomic_inc(&mCount);
    }

    inline void decStrong(const void *id) const {
        if (android_atomic_dec(&mCount) == 1) {
            delete static_cast<const T *>(this);
        }
    }

    //! DEBUGGING ONLY: Get current strong ref count.
    inline int getStrongCount() const {
        return mCount;
    }

protected:
    inline ~LightRefBase() {}

private:
    mutable int mCount;
};

// ---------------------------------------------------------------------------

template<typename T>
class sp {
public:
    typedef typename RefBase::weakref_type weakref_type;

    inline sp() : m_ptr(0) {}

    sp(T *other);

    sp(const sp<T> &other);

    template<typename U>
    sp(U *other);

    template<typename U>
    sp(const sp<U> &other);

    ~sp();

    // Assignment

    sp &operator=(T *other);

    sp &operator=(const sp<T> &other);

    template<typename U>
    sp &operator=(const sp<U> &other);

    template<typename U>
    sp &operator=(U *other);

    //! Special optimization for use by ProcessState (and nobody else).
    void force_set(T *other);

    // Reset

    void clear();

    // Accessors

    inline T &operator*() const { return *m_ptr; }

    inline T *operator->() const { return m_ptr; }

    inline T *get() const { return m_ptr; }

    // Operators

    COMPARE(==)

    COMPARE(!=)

    COMPARE(>)

    COMPARE(<)

    COMPARE(<=)

    COMPARE(>=)

private:
    template<typename Y> friend
    class sp;

    template<typename Y> friend
    class wp;

    // Optimization for wp::promote().
    sp(T *p, weakref_type *refs);

    T *m_ptr;
};

template<typename T>
TextOutput &operator<<(TextOutput &to, const sp<T> &val);

// ---------------------------------------------------------------------------

template<typename T>
class wp {
public:
    typedef typename RefBase::weakref_type weakref_type;

    inline wp() : m_ptr(0) {}

    wp(T *other);

    wp(const wp<T> &other);

    wp(const sp<T> &other);

    template<typename U>
    wp(U *other);

    template<typename U>
    wp(const sp<U> &other);

    template<typename U>
    wp(const wp<U> &other);

    ~wp();

    // Assignment

    wp &operator=(T *other);

    wp &operator=(const wp<T> &other);

    wp &operator=(const sp<T> &other);

    template<typename U>
    wp &operator=(U *other);

    template<typename U>
    wp &operator=(const wp<U> &other);

    template<typename U>
    wp &operator=(const sp<U> &other);

    void set_object_and_refs(T *other, weakref_type *refs);

    // promotion to sp

    sp<T> promote() const;

    // Reset

    void clear();

    // Accessors

    inline weakref_type *get_refs() const { return m_refs; }

    inline T *unsafe_get() const { return m_ptr; }

    // Operators

    COMPARE_WEAK(==)

    COMPARE_WEAK(!=)

    COMPARE_WEAK(>)

    COMPARE_WEAK(<)

    COMPARE_WEAK(<=)

    COMPARE_WEAK(>=)

    inline bool operator==(const wp<T> &o) const {
        return (m_ptr == o.m_ptr) && (m_refs == o.m_refs);
    }

    template<typename U>
    inline bool operator==(const wp<U> &o) const {
        return m_ptr == o.m_ptr;
    }

    inline bool operator>(const wp<T> &o) const {
        return (m_ptr == o.m_ptr) ? (m_refs > o.m_refs) : (m_ptr > o.m_ptr);
    }

    template<typename U>
    inline bool operator>(const wp<U> &o) const {
        return (m_ptr == o.m_ptr) ? (m_refs > o.m_refs) : (m_ptr > o.m_ptr);
    }

    inline bool operator<(const wp<T> &o) const {
        return (m_ptr == o.m_ptr) ? (m_refs < o.m_refs) : (m_ptr < o.m_ptr);
    }

    template<typename U>
    inline bool operator<(const wp<U> &o) const {
        return (m_ptr == o.m_ptr) ? (m_refs < o.m_refs) : (m_ptr < o.m_ptr);
    }

    inline bool operator!=(const wp<T> &o) const { return m_refs != o.m_refs; }

    template<typename U>
    inline bool operator!=(const wp<U> &o) const { return !operator==(o); }

    inline bool operator<=(const wp<T> &o) const { return !operator>(o); }

    template<typename U>
    inline bool operator<=(const wp<U> &o) const { return !operator>(o); }

    inline bool operator>=(const wp<T> &o) const { return !operator<(o); }

    template<typename U>
    inline bool operator>=(const wp<U> &o) const { return !operator<(o); }

private:
    template<typename Y> friend
    class sp;

    template<typename Y> friend
    class wp;

    T *m_ptr;
    weakref_type *m_refs;
};

template<typename T>
TextOutput &operator<<(TextOutput &to, const wp<T> &val);

#undef COMPARE
#undef COMPARE_WEAK

// ---------------------------------------------------------------------------
// No user serviceable parts below here.

template<typename T>
sp<T>::sp(T *other)
        : m_ptr(other) {
    if (other) other->incStrong(this);
}

template<typename T>
sp<T>::sp(const sp<T> &other)
        : m_ptr(other.m_ptr) {
    if (m_ptr) m_ptr->incStrong(this);
}

template<typename T>
template<typename U>
sp<T>::sp(U *other) : m_ptr(other) {
    if (other) other->incStrong(this);
}

template<typename T>
template<typename U>
sp<T>::sp(const sp<U> &other)
        : m_ptr(other.m_ptr) {
    if (m_ptr) m_ptr->incStrong(this);
}

template<typename T>
sp<T>::~sp() {
    if (m_ptr) m_ptr->decStrong(this);
}

template<typename T>
sp<T> &sp<T>::operator=(const sp<T> &other) {
    T *otherPtr(other.m_ptr);
    if (otherPtr) otherPtr->incStrong(this);
    if (m_ptr) m_ptr->decStrong(this);
    m_ptr = otherPtr;
    return *this;
}

template<typename T>
sp<T> &sp<T>::operator=(T *other) {
    if (other) other->incStrong(this);
    if (m_ptr) m_ptr->decStrong(this);
    m_ptr = other;
    return *this;
}

template<typename T>
template<typename U>
sp<T> &sp<T>::operator=(const sp<U> &other) {
    U *otherPtr(other.m_ptr);
    if (otherPtr) otherPtr->incStrong(this);
    if (m_ptr) m_ptr->decStrong(this);
    m_ptr = otherPtr;
    return *this;
}

template<typename T>
template<typename U>
sp<T> &sp<T>::operator=(U *other) {
    if (other) other->incStrong(this);
    if (m_ptr) m_ptr->decStrong(this);
    m_ptr = other;
    return *this;
}

template<typename T>
void sp<T>::force_set(T *other) {
    other->forceIncStrong(this);
    m_ptr = other;
}

template<typename T>
void sp<T>::clear() {
    if (m_ptr) {
        m_ptr->decStrong(this);
        m_ptr = 0;
    }
}

template<typename T>
sp<T>::sp(T *p, weakref_type *refs)
        : m_ptr((p && refs->attemptIncStrong(this)) ? p : 0) {
}

template<typename T>
inline TextOutput &operator<<(TextOutput &to, const sp<T> &val) {
    to << "sp<>(" << val.get() << ")";
    return to;
}

// ---------------------------------------------------------------------------

template<typename T>
wp<T>::wp(T *other)
        : m_ptr(other) {
    if (other) m_refs = other->createWeak(this);
}

template<typename T>
wp<T>::wp(const wp<T> &other)
        : m_ptr(other.m_ptr), m_refs(other.m_refs) {
    if (m_ptr) m_refs->incWeak(this);
}

template<typename T>
wp<T>::wp(const sp<T> &other)
        : m_ptr(other.m_ptr) {
    if (m_ptr) {
        m_refs = m_ptr->createWeak(this);
    }
}

template<typename T>
template<typename U>
wp<T>::wp(U *other)
        : m_ptr(other) {
    if (other) m_refs = other->createWeak(this);
}

template<typename T>
template<typename U>
wp<T>::wp(const wp<U> &other)
        : m_ptr(other.m_ptr) {
    if (m_ptr) {
        m_refs = other.m_refs;
        m_refs->incWeak(this);
    }
}

template<typename T>
template<typename U>
wp<T>::wp(const sp<U> &other)
        : m_ptr(other.m_ptr) {
    if (m_ptr) {
        m_refs = m_ptr->createWeak(this);
    }
}

template<typename T>
wp<T>::~wp() {
    if (m_ptr) m_refs->decWeak(this);
}

template<typename T>
wp<T> &wp<T>::operator=(T *other) {
    weakref_type *newRefs =
            other ? other->createWeak(this) : 0;
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = other;
    m_refs = newRefs;
    return *this;
}

template<typename T>
wp<T> &wp<T>::operator=(const wp<T> &other) {
    weakref_type *otherRefs(other.m_refs);
    T *otherPtr(other.m_ptr);
    if (otherPtr) otherRefs->incWeak(this);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = otherRefs;
    return *this;
}

template<typename T>
wp<T> &wp<T>::operator=(const sp<T> &other) {
    weakref_type *newRefs =
            other != NULL ? other->createWeak(this) : 0;
    T *otherPtr(other.m_ptr);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = newRefs;
    return *this;
}

template<typename T>
template<typename U>
wp<T> &wp<T>::operator=(U *other) {
    weakref_type *newRefs =
            other ? other->createWeak(this) : 0;
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = other;
    m_refs = newRefs;
    return *this;
}

template<typename T>
template<typename U>
wp<T> &wp<T>::operator=(const wp<U> &other) {
    weakref_type *otherRefs(other.m_refs);
    U *otherPtr(other.m_ptr);
    if (otherPtr) otherRefs->incWeak(this);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = otherRefs;
    return *this;
}

template<typename T>
template<typename U>
wp<T> &wp<T>::operator=(const sp<U> &other) {
    weakref_type *newRefs =
            other != NULL ? other->createWeak(this) : 0;
    U *otherPtr(other.m_ptr);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = newRefs;
    return *this;
}

template<typename T>
void wp<T>::set_object_and_refs(T *other, weakref_type *refs) {
    if (other) refs->incWeak(this);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = other;
    m_refs = refs;
}

template<typename T>
sp<T> wp<T>::promote() const {
    return sp<T>(m_ptr, m_refs);
}

template<typename T>
void wp<T>::clear() {
    if (m_ptr) {
        m_refs->decWeak(this);
        m_ptr = 0;
    }
}

template<typename T>
inline TextOutput &operator<<(TextOutput &to, const wp<T> &val) {
    to << "wp<>(" << val.unsafe_get() << ")";
    return to;
}

}; // namespace trinity

#endif //TRINITY_REF_BASE_H
