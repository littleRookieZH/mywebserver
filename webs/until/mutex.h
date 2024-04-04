#ifndef _WEBS_MUTEX_H_
#define _WEBS_MUTEX_H_

#include <semaphore.h>
#include <stdint.h>
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
        /* 加锁 */

        /* 解锁 */
    private:
        T& 
    };

    /* 局部读锁模板实现 */
    template <class T>
    struct ReadScopedLockImpl
    {
    };

    /* 局部写锁模板实现 */
    template <class T>
    struct WriteScopedLockImpl
    {
    };

    /* 互斥锁 */
    class Mutex : Noncopyable
    {
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