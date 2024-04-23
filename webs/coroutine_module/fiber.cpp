#include "fiber.h"
#include "../log_module/log.h"
#include "../config_module/config.h"
#include "../util_module/macro.h"
#include <atomic>
#include "scheduler.h"
namespace webs {

// 创建日志信息
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");
// 创建原子变量
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};
// 创建线程的协程的智能指针对象和协程的指针
static thread_local Fiber *t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;
// 获取协程运行时栈大小的配置信息
webs::ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

/* 动态分配内存的类 */
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) {
        return malloc(size);
    }
    static void Dealloc(void *vp, size_t size) {
        return free(vp);
    }
};

// 重命名
using StackAllocator = MallocStackAllocator;

// 私有构造
Fiber::Fiber() {
    // 修改状态，设置当前的线程的协程指针(t_fiber)
    m_state = EXEC;
    Fiber::SetThis(this);
    // 保存当前协程的上下文
    if (getcontext(&m_ctx)) {
        WEBS_ASSERT2(false, "getcontext");
    }
    // 增加协程计数
    ++s_fiber_count;
    // 将 "Fiber::Fiber main" 输出到debug日志
    WEBS_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

/**
     * 构造函数
     * 协程执行函数
     * 协程栈大小
     * 
     * 问题：一：如何使用；二：协程调度的思路不清
     *  */
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) :
    m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;
    // 确定运行时栈的大小，并分配内存
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);
    // 保存协程上下文，并设置上下文属性
    ;
    if (getcontext(&m_ctx)) {
        WEBS_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    // 切换协程运行：一个是切换协程管理器的主协程，另一个是切换线程的主协程
    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    // 输出日志信息
    WEBS_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

/* 析构函数；如果是主协程，将当前协程指针置为空；如果是子协程，释放栈空间 */
Fiber::~Fiber() {
    --s_fiber_count; // 有个疑问如果是主协程为什么也需要 --
    if (m_stack) {
        // 子协程
        // WEBS_ASSERT();
        WEBS_ASSERT(m_state == INIT || m_state == TERM || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // 确认是否为主协程
        WEBS_ASSERT(!m_cb);
        WEBS_ASSERT(m_state == EXEC);
        Fiber *cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    WEBS_LOG_DEBUG(g_logger)
        << "Fiber::~Fiber id = " << m_id << " total = " << s_fiber_count;
}

/**
     * 重置协程执行函数，并重置状态
     * 协程的状态：INIT EXCEPT TERM
     * 重置后状态为INIT
     * 为了充分利用内存，一个协程执行完，但是内存没有释放，此时可以重置内存，让其重新成为一个执行栈
     */
void Fiber::reset(std::function<void()> cb) {
    WEBS_ASSERT(m_stack);
    WEBS_ASSERT(m_state == INIT || m_state == TERM || m_state == EXCEPT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        WEBS_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    // 重置了上下文的栈起始地址
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    // 这里是可以执行的，make只是设置，并不会去执行函数
    m_state = INIT;
}

/**
  * 将当前操作的协程与当前正在运行的协程进行切换
  * 将上下文保存调度器的协程中，切换到当前协程执行
  * 当前状态 != EXEC
  */
void Fiber::swapIn() {
    SetThis(this);
    WEBS_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        WEBS_ASSERT2(false, "swapcontext");
    }
}

/**
  * 保存当前协程的上下文，切换到后台协程的上下文继续执行
  * 疑问：这里的协程状态不需要切为exec吗
  */
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        WEBS_ASSERT2(false, "swapcontext");
    }
}

/**
  * 将当前线程切换到执行状态
  * 将上下文信息保存到线程的协程中，切换到当前协程的上下文继续执行
  */
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    // -> . 的优先级高于 &
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        WEBS_ASSERT2(false, "swapcontext");
    }
}

/* 保存上下文到当前的运行的协程中，并将控制流切换到线程持有的协程继续执行 */
void Fiber::back() {
    SetThis(t_threadFiber.get());
    // t_threadFiber->m_state = EXEC;
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        WEBS_ASSERT2(false, "swapcontext");
    }
}

/* 设置当前线程的运行协程 */
void Fiber::SetThis(Fiber *f) {
    t_fiber = f;
}

/* 返回当前线程的协程 */
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    // 如果没有，则创建主协程。同时会赋值t_fiber
    Fiber::ptr main_fiber(new Fiber);
    // 检查t_fiber是否等于main_fiber中的fiber对象
    WEBS_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

/* 将当前协程切换到后台，并设置状态为READY */
void Fiber::YieldToTeady() {
    // 获取当前运行的协程，并检查是否是运行态
    Fiber::ptr cur = GetThis();
    WEBS_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    // 修改状态并切换
    cur->swapOut();
}

/* 将当前协程切换到后台，并设置状态为HOLD */
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    WEBS_ASSERT(cur->m_state == EXEC);
    //cur->m_state = HOLD;
    cur->swapOut();
}

/* 返回当前协程的总数量 */
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}
/**
     * 协程执行函数
     * 切换到调度器中的协程上下文继续执行
    */
void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    WEBS_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        WEBS_LOG_ERROR(g_logger) << "Fiber except = " << ex.what()
                                 << "fiber_id = " << cur->getId() << std::endl
                                 << webs::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        WEBS_LOG_ERROR(g_logger) << "Fiber except = "
                                 << "fiber_id = " << cur->getId() << std::endl
                                 << webs::BacktraceToString();
    }
    Fiber *arwcur = cur.get();
    // cur的计数至少是大于1的
    cur.reset();       // 如果没有reset，swapOut之后，cur没有释放，会导致最终协程无法释放
    arwcur->swapOut(); // 当前协程执行完之后，又回到主协程执行
    WEBS_ASSERT2(false, "never get fiber id = " + std::to_string(cur->getId()));
}

/**
     * 协程执行函数
     * 切换到当前线程的协程上下文继续执行
    */
void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        WEBS_LOG_ERROR(g_logger) << "Fiber except = " << ex.what()
                                 << " Fiber_id = " << cur->m_id << std::endl
                                 << webs::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        WEBS_LOG_ERROR(g_logger) << "Fiber except = "
                                 << " Fiber_id = " << cur->m_id << std::endl
                                 << webs::BacktraceToString();
    }
    Fiber *awrcur = cur.get();
    cur.reset();
    awrcur->back();
    WEBS_ASSERT2(false, "never reach fiber id = " + std::to_string(cur->getId()));
}

/* 返回当前协程的id */
uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->m_id;
    }
    return 0;
}

} // namespace webs