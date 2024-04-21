#ifndef __WEBS_FIBER_H__
#define __WEBS_FIBER_H__
#include <memory>
#include <ucontext.h>
#include <functional>

namespace webs {
/* 协程调度类 */
class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;

public:
    typedef std::shared_ptr<Fiber> ptr;
    // 协程的状态
    enum State {
        /// @brief  初始状态
        INIT,
        /// @brief 暂停状态
        HOLD,
        /// @brief 执行中状态
        EXEC,
        /// @brief 终止状态
        TERM,
        /// @brief 可执行状态
        READY,
        /// @brief 异常状态
        EXCEPT
    };

private:
    /* 每个线程第一个协程的构造 */
    Fiber();

public:
    /**
     * 构造函数
     * 协程执行函数
     * 协程栈大小
     * 是否在主协程上调度
     *  */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_call = false);

    /* 析构函数 */
    ~Fiber();

    /**
     * 重置协程执行函数，并重置状态
     * 协程的状态：INIT EXCEPT TERM
     * 重置后状态为INIT
     */
    void reset(std::function<void()> cb);

    /**
     * 将当前协程的状态切换为运行态
     * 保存调度器中的协程上下文，切换到当前协程执行
     * 当前状态 != EXEC
    */
    void swapIn();

    /* 将当前协程切换为后台运行 */
    void swapOut();

    /**
     * 将当前线程切换到执行状态
     * 保存当前协程的上下文，切换到当前线程的协程
     * 线程自己有一个主协程(thread_local修饰的)：一般为创建的第一个协程
    */
    void call();

    /**
     * 将当前线程切换到后台
     * 执行的为该协程
     * 返回到线程的主协程
     */
    void back();

    /**
     * 返回协程id
    */
    uint64_t getId() const {
        return m_id;
    }

    /**
     * 返回协程状态
    */
    State getState() const {
        return m_state;
    }

public:
    /* 设置当前线程的运行协程 */
    static void SetThis(Fiber *f);

    /* 返回当前线程的协程 */
    static Fiber::ptr GetThis();

    /* 将当前协程切换到后台，并设置状态为READY */
    static void YieldToTeady();

    /* 将当前协程切换到后台，并设置状态为HOLD */
    static void YieldToHold();

    /* 返回当前协程的总数量 */
    static uint64_t TotalFibers();

    /**
     * 协程执行函数
     * 切换到调度器中的协程上下文继续执行
    */
    static void MainFunc();

    /**
     * 协程执行函数
     * 切换到当前线程的协程上下文继续执行
    */
    static void CallerMainFunc();

    /* 返回当前协程的id */
    static uint64_t GetFiberId();

private:
    // 协程id
    uint64_t m_id = 0;
    // 协程运行时栈的大小
    uint32_t m_stacksize = 0;
    // 协程状态
    State m_state = INIT;
    // 协程的上下文
    ucontext_t m_ctx;
    // 协程运行栈指针
    void *m_stack = nullptr;
    // 协程的可执行函数
    std::function<void()> m_cb;
};
} // namespace webs

#endif