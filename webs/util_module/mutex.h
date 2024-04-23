#ifndef __WEBS_MUTEX_H__
#define __WEBS_MUTEX_H__

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include <atomic>
#include <mutex>

#include "Noncopyable.h"

namespace webs {
/* 信号量 */
class Semaphore : Noncopyable {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();
    /* 获取信号量 */
    void wait();
    /* 释放信号量 */
    void notify();

private:
    sem_t m_semaphore;
};

/* 局部锁的模板实现；T的类型不同意味着使用的锁也不同 */
template <class T>
struct ScopedLockImpl {
public:
    ScopedLockImpl(T &mutex) :
        m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    };
    ~ScopedLockImpl() {
        unlock();
    };
    /* 加锁 */
    void lock() {
        if (!m_locked) {
            // 调用T类型对象的lock
            m_mutex.lock();
            m_locked = true;
        }
    };
    /* 解锁 */
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // mutex：封装互斥量；注意这里是必须是引用，如果是值传递就无法保证每次获取的都是同一把锁
    T &m_mutex;
    //
    bool m_locked;
};

/* 局部读锁模板实现 */
template <class T>
struct ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T &mutex) :
        m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl() {
        unlock();
    }
    /* 上读锁 */
    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    /* 释放读锁 */
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // mutex
    T &m_mutex;
    // 是否已上锁
    bool m_locked;
};

/* 局部写锁模板实现 */
template <class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T &mutex) :
        m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    };
    ~WriteScopedLockImpl() {
        unlock();
    };
    /* 上写锁 */
    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    /* 解写锁 */
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // 写锁
    T &m_mutex;
    // 是否已上锁
    bool m_locked;
};

/* 互斥锁 --- 模板锁的特例化实现 */
class Mutex : Noncopyable {
public:
    /// 局部锁
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    };
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    };
    /* 加锁 */
    void lock() {
        pthread_mutex_lock(&m_mutex);
    };
    /* 解锁 */
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    };

private:
    pthread_mutex_t m_mutex;
};

/* 空锁(用于调试) */
class NullMutex : Noncopyable {
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex(){};
    ~NullMutex(){};
    void lock(){};
    void unlock(){};
};

/* 读写互斥量 */
class RWMutex : Noncopyable {
public:
    // 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    // 局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }
    /* 上读锁 */
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }
    /* 上写锁 */
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }
    /* 解锁 */
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

/* 空读写锁(用于调试)*/
class NullRWMutex : Noncopyable {
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex(){};
    ~NullRWMutex(){};
    void lock(){};
    void unlock(){};
};

/* 自旋锁 */
class Spinlock : Noncopyable {
public:
    // 局部锁
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() {
        /* 0，表示创建的自旋锁是进程内共享的;非0，表示进程间共享 */
        pthread_spin_init(&m_lock, 0);
    };
    ~Spinlock() {
        pthread_spin_destroy(&m_lock);
    };
    /* 上锁 */
    void lock() {
        pthread_spin_lock(&m_lock);
    };
    /* 解锁 */
    void unlock() {
        pthread_spin_unlock(&m_lock);
    };

private:
    // 自旋锁
    pthread_spinlock_t m_lock;
};

/* 原子锁 */
class CASLock : Noncopyable {
public:
    // 局部锁
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_lock.clear();
    }
    ~CASLock() {
    }

    /* 上锁 */
    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&m_lock,
                                                      std::memory_order_acquire))
            ;
    }

    /* 解锁 */
    void unlock() {
        std::atomic_flag_clear_explicit(&m_lock, std::memory_order_release);
    }

private:
    int i = 1;
    // 原子状态；volatile关键字声明变量每次读写操作都先从内存中获取（非寄存器）
    volatile std::atomic_flag m_lock;
};

/// 还有协程锁没有写
}; // namespace webs

#endif