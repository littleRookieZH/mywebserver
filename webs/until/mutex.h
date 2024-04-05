#ifndef _WEBS_MUTEX_H_
#define _WEBS_MUTEX_H_

#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>
#include <mutex>

#include "Noncopyable.h"

namespace webs
{
    /* 信号量 */
    class Semaphore : Noncopyable
    {
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

    /* 局部锁的模板实现 */
    template <class T>
    struct ScopedLockImpl
    {
    public:
        ScopedLockImpl(T &mutex) : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        };
        ~ScopedLockImpl()
        {
            unlock();
        };
        /* 加锁 */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        };
        /* 解锁 */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        // mutex：封装互斥量
        T &m_mutex;
        //
        bool m_locked;
    };

    /* 局部读锁模板实现 */
    template <class T>
    struct ReadScopedLockImpl
    {
    public:
        ReadScopedLockImpl(T &mutex) : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        ~ReadScopedLockImpl()
        {
            unlock();
        }
        /* 上读锁 */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }
        /* 释放读锁 */
        void unlock()
        {
            if (m_locked)
            {
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
    struct WriteScopedLockImpl
    {
    public:
        WriteScopedLockImpl(T &mutex) : m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        };
        ~WriteScopedLockImpl()
        {
            unlock();
        };
        /* 上写锁 */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }
        /* 解写锁 */
        void unlock()
        {
            if (m_locked)
            {
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

    /* 互斥锁 */
    class Mutex : Noncopyable
    {
        public:
            /// 局部锁
            typedef ScopedLockImpl<Mutex> Lock;
            Mutex(){
                pthread_mutex_init(&m_mutex, nullptr);
            };
            ~Mutex(){
                pthread_mutex_destroy(&m_mutex);
            };
            /* 加锁 */
            void lock(){
                pthread_mutex_lock(&m_mutex);
            };
            /* 解锁 */
            void unlock(){
                pthread_mutex_unlock(&m_mutex);
            };

        private:
            pthread_mutex_t m_mutex;
    };

    /* 空锁(用于调试) */
    class NullMutex : Noncopyable
    {
    };

    /* 读写互斥量 */
    class RWMutex : Noncopyable
    {
    };

    /* 空读写锁(用于调试)*/
    class NullRWMutex : Noncopyable
    {
    };

    /* 自旋锁 */
    class Spinlock : Noncopyable
    {
    };

    /* 原子锁 */
    class CASLock : Noncopyable
    {
    };

    /// 还有协程锁没有写
};

#endif