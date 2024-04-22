#include "scheduler.h"
#include "../log_module/log.h"
#include "../util/macro.h"
#include "hook.h"

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
/* 返回当前协程调度器的调度协程 */
Fiber *Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

/* 通知协程调度器有任务了；可以自定义调度器的执行方式(如何调度) */
void Scheduler::tickle() {
    WEBS_LOG_INFO(g_logger) << "tickle";
}

/* 协程无任务可以调度时，执行idle协程 */
void Scheduler::idle() {
    WEBS_LOG_INFO(g_logger) << "idle";
    while (!stopping()) { //如果不能停止，设置当前协程的状态，并切换其他协程执行
        webs::Fiber::YieldToHold();
    }
}

/* 设置当前的协程调度器 */
void Scheduler::setThis() {
    t_scheduler = this;
}

/* 启动协程调度器 */
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!stopping) {
        return;
    }
    WEBS_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    for (int i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

/* 停止协程调度器 */
void Scheduler::stop() {
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
        WEBS_LOG_INFO(g_logger) << this << "stopped"; // 这里怎么直接将指针传进去了？
        m_stopping = true;
        if (stopping()) {
            return;
        }
    }

    // 分为两种线程：一种是use_caller，；一种是非use
    if (m_rootThread != -1) {
        WEBS_ASSERT(GetThis() == this); // 只能创建use_caller的调度器执行
    } else {
        WEBS_ASSERT(GetThis() != this); // 不能再创建该线程的调度器执行stop 有点疑惑。。
    }
    m_stopping = true;
    for (int i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    if (m_rootFiber) {
        tickle();
    }
    // 不太理解
    if (m_rootFiber) {
        if (!stopping) {
            m_rootFiber->call();
        }
    }
}

/* 返回是否可以停止 */
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

/**
 * 作用：将当前正在执行的协程重新加入工作队列；
 * 解释了：cb_fiber->m_state = Fiber::HOLD; cb_fiber.reset();的效果。并不是修改完，该协程就被释放了 --->该协程会被重新加入工作队列
 * */
void Scheduler::switchTo(int thread = -1) {
    // 在指定的线程中执行工作协程，执行完再切换进来其他协程执行
    WEBS_ASSERT(Scheduler::GetThis() != nullptr);
    if (Scheduler::GetThis() == this) {
        if (thread == -1 || webs::GetThreadId() == thread) {
            return;
        }
    }
    schedule(webs::Fiber::GetThis(), thread);
    webs::Fiber::YieldToHold();
}

/* 输出协程的信息 */
std::ostream &Scheduler::dump(std::ostream &os) {
    os << "[Scheduler name = " << m_name
       << " size = " << m_threadCount
       << " active_count = " << m_activeThreadCount
       << " idle_count = " << m_activeThreadCount
       << " stopping = " << m_stopping
       << " ]" << std::endl
       << "    ";
    for (int i = 0; i < m_threadIds.size(); ++i) {
        if (i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

/* 协程调度函数 */
void Scheduler::run() {
    // 设置调度器
    WEBS_LOG_DEBUG(g_logger) << m_name << " run";
    set_hook_enable(true);
    setThis();
    // 如果线程id 不等于 调度器中的线程id
    if (webs::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;
    FiberAndThread ft;
    // 根据线程id查找可以运行的工作任务
    while (true) {
        ft.reset();
        // 是否采用用户自定义的调度方法
        bool tickle_me = false;
        // 是否有线程在工作。工作完需要数量减一
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            // 查找可以执行的任务
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                // 如果工作任务指定的线程不是当前线程，通知其他线程执行，继续查找
                if (it->thread != -1 && it->thread != webs::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }
                // 找到可以执行任务；校验并设置任务
                // 校验
                WEBS_ASSERT(it->cb != nullptr || it->fiber != nullptr);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                // 设置任务
                ft = *it;
                it = m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end(); // it != m_fibers.end()表示：还有任务要执行
        }

        // 如果设置了tickle，通知一下其他线程
        if (tickle_me) {
            tickle();
        }
        // 一：任务是协程，执行--> 执行完，根据工作状态执行不同的策略
        if (ft.fiber && ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
            ft.fiber->swapIn();
            // 协程执行完切回当前线程的下文需要将活跃数--
            --m_activeThreadCount;
            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) { // 二：任务是function，创建执行function的协程，并执行 --> 执行完，根据状态执行不同的策略
            if (cb_fiber) {
                cb_fiber->reset(ft.cb); // 重置上下文
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            // 切回当前上下文执行
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset(); // 将协程重置；并不会导致工作协程被改变(仅仅是引用数减一)
            } else if (cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT) {
                // 将协程的上下文重置
                cb_fiber->reset(nullptr);
            } else {
                // 这里在返回来之前，会将工作协程加入工作队列，所以不能使用cb_fiber->reset(nullptr) --->会将工作协程的上下文清空
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset(); // 将协程重置；并不会导致工作协程被改变(仅仅是引用数减一)
            }
        } else { // 三：无任务。执行idle函数。idle可以根据不同的使用场景执行不同的策略，比如：切换其他线程去执行、当前线程循环等待....
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                WEBS_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            ++m_activeThreadCount;
            idle_fiber->swapIn();
            --m_activeThreadCount;
            if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler *target) :
    m_caller(target) {
    if (target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher() {
    if (m_caller) {
        m_caller->switchTo();
    }
}

} // namespace webs