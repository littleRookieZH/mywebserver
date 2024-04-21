#include "scheduler.h"
#include "../log_module/log.h"
#include "../util/macro.h"
namespace webs {
// 日志
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");
// 当前线程的协程调度器指针
static thread_local Scheduler *t_scheduler = nullptr;
// 调度器的主协程指针
static thread_local Fiber *t_scheduler_fiber = nullptr;

/* 构造函数：线程的数量、是否将当前线程加入协程调度器、协程调度器的名字 */
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) :
    m_name(name) {
    WEBS_ASSERT(threads > 0);
    // 分为两种情况：一种是需要将当前线程加入到协程调度器中 use_caller = true
    if (use_caller) {
        // 给线程创建主协程
        webs::Fiber::GetThis();
        // 没必要再多分配一个线程，因为当前线程也可以作为其中之一
        --threads;
        // 如果GetThis()不为nullptr，说明当前线程已经有一个协程调度器了
        WEBS_ASSERT(GetThis() == nullptr);
        t_scheduler = this;
        // 使用use_caller，意味着线程的main协程不能参与到run方法，负责调度子协程。因此需要创建一个子协程执行run
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        // 更新调度器的协程指针
        t_scheduler_fiber = m_rootFiber.get();
        // 获取当前线程id，可以用与区分use_caller和非use_caller模式
        m_rootThread = webs::GetThreadId();
        m_threadIds.push_back(m_rootThread);
        webs::Thread::SetName(m_name);
    } else {
        // 非use_caller模式，全部设置为-1
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

/* 析构函数 */
Scheduler::~Scheduler() {
    WEBS_ASSERT(m_stopping);
    if (GetThis() == this) { // 如果线程所指向的调度器与当前调度器相同
        t_scheduler = nullptr;
    }
}

Scheduler *Scheduler::GetThis() {
    return t_scheduler;
}

    /* 启动协程调度器 */
    void Scheduler::start(){
        MutexType::Lock lock(m_mutex);
        if(!stopping){
            return;
        }
        WEBS_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);
        for(int i = 0; i < m_threadCount; ++i){
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    /* 停止协程调度器 */
    void Scheduler::stop(){
        m_autoStop = true;
        if(m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)){
            WEBS_LOG_INFO(g_logger) << this << "stopped"; // 这里怎么直接将指针传进去了？
            m_stopping = true;
            if(stopping()){
                return;
            }
        }

        // 分为两种线程：一种是use_caller，；一种是非use
        if(m_rootThread != -1){
            WEBS_ASSERT(GetThis() == this);// 只能创建use_caller的调度器执行
        }else{
            WEBS_ASSERT(GetThis() != this);// 不能再创建该线程的调度器执行stop 有点疑惑。。
        }
        m_stopping = true;
        for(int i = 0; i < m_threadCount; ++i){
            tickle();
        }
        if(m_rootFiber){
            tickle();
        }
        // 不太理解
        if(m_rootFiber){
            if(!stopping){
                m_rootFiber->call();
            }
        }
    }
    
        /* 返回是否可以停止 */
    bool Scheduler::stopping(){
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

} // namespace webs