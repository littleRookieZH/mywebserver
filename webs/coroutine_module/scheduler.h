/**
 * 当我们的协程需要跨线程的时候，就需要一个调度器去分配具体的线程执行协程
 * 线程池，分配一组线程
 * 协程调度器，将协程指定到相应的线程上去执行
 * 
 * 读写分离用的少，所以使用互斥锁即可
 * 
*/
#ifndef __WEBS_SCHEDULER_H__
#define __WEBS_SCHEDULER_H__
#include <list>
#include <memory>
#include <vector>
#include "../util/mutex.h"
#include "fiber.h"
#include "../thread_module/thread.h"
namespace webs {

/**
 * 协程调度器：
 * 封装的是N个线程M个协程的调度器，内部有一个线程池，用于支持协程在线程池里面切换
*/
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /* 构造函数：线程的数量、是否将当前线程加入协程调度器、协程调度器的名字 */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");

    /* 析构函数 */
    virtual ~Scheduler();

    /* 返回当前协程调度器的名字 */
    const std::string &getName() const {
        return m_name;
    }

    /* 返回当前的协程调度器 */
    static Scheduler *GetThis();

    /* 返回当前协程调度器的调度协程 */
    static Fiber *GetMainFiber();

    /* 启动协程调度器 */
    void start();

    /* 停止协程调度器 */
    void stop();

    /* 调度协程，可以是function，也可以是协程 */
    template <class FiberOrCb>
    bool schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNolock(fc, thread);
        }
        // 可以执行自定义的协程调度器的调度方法
        if (need_tickle) {
            tickle();
        }
        return need_tickle;
    }

    /* 批量调度协程 */
    template <class InputIterator>
    bool schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNolock(&*begin, -1) || need_tickle;
            }
        }
        if (need_tickle) {
            tickle();
        }
        return need_tickle;
    }
    void switchTo(int thread = -1);
    std::ostream &dump(std::ostream &os);

protected:
    /* 通知协程调度器有任务了；可以自定义调度器的执行方式(如何调度) */
    virtual void tickle();

    /* 协程调度函数 */
    void run();

    /* 返回是否可以停止 */
    virtual bool stopping();

    /* 协程无任务可以调度时，执行idle协程 */
    virtual void idle();

    /* 设置当前的协程调度器 */
    void setThis();

    /* 是否有空闲线程 */
    bool hasIdleThreads() {
        return m_idleThreadCount > 0;
    }

private:
    /* 协程调度器启动(无锁)，将协程或者function加入调度器 */
    template <class FiberOrCb>
    bool scheduleNolock(FiberOrCb fc, int thread = -1) {
        bool need_tickle = m_fibers.empty(); // 如果为空，唤醒线程
        FiberAndThread ft(fc, thread);
        if (ft.cb || ft.fiber) {    // 要么是协程，要么是函数指针
            m_fibers.push_back(ft); // 加入等待执行的协程队列中
        }
        return need_tickle; // true表示以前是没有任务的，现在任务来了，去唤醒线程。在从协程队列中取出协程
    }

private:
    struct FiberAndThread {
        // 协程
        Fiber::ptr fiber;
        // 协程执行函数
        std::function<void()> cb;
        // 线程id
        int thread;

        /* 协程智能指针对象与线程 */
        FiberAndThread(Fiber::ptr f, int thr) :
            fiber(f), thread(thr) {
        }
        /* 协程智能指针对象的指针与线程 */
        FiberAndThread(Fiber::ptr *f, int thr) :
            thread(thr) {
            // swap后引用计数不会增加
            fiber.swap(*f);
        }
        /* function与线程 */
        FiberAndThread(std::function<void()> f, int thr) :
            cb(f), thread(thr) {
        }
        /* function指针与线程 */
        FiberAndThread(std::function<void()> *f, int thr) :
            thread(thr) {
            cb.swap(*f);
        }
        /* 无参构造 */
        FiberAndThread() :
            thread(-1) {
        }
        /* 重置数据 */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    // 互斥锁
    MutexType m_mutex;
    // 线程池
    std::vector<Thread::ptr> m_threads;
    // 待执行的协程队列；可以是协程也可以是function函数
    std::list<FiberAndThread> m_fibers;
    // use_caller为true时，有效；用于调度协程
    Fiber::ptr m_rootFiber;
    // 协程调度器的名字
    std::string m_name;

protected:
    // 协程调度器包含的线程id
    std::vector<int> m_threadIds;
    // 线程数量
    size_t m_threadCount = 0;
    // 工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    // 空闲线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    // 是否正在停止
    bool m_stopping = true;
    // 是否自动停止
    bool m_autoStop = false;
    // 主线程id (use_caller)
    int m_rootThread = 0;
};
} // namespace webs

#endif