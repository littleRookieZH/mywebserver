#include "iomanager.h"
#include "../util_module/macro.h"

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
namespace webs {

static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

// 声明一个空枚举；强转为该类型进行输出
enum EpollCtop {};

/* 输出epoll_ctl */
std::ostream &operator<<(std::ostream &os, const EpollCtop &op) {
    switch ((int)op) {
#define XX(ctl) \
    case ctl:   \
        return os << #ctl;

        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
    default:
        return os << (int)op;
    }
#undef XX
}

/* 输出epoll_events */
std::ostream &operator<<(std::ostream &os, EPOLL_EVENTS events) {
    if (!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E)          \
    if (E & events) {  \
        if (!first) {  \
            os << "|"; \
        }              \
        os << #E;      \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLEXCLUSIVE);
    XX(EPOLLWAKEUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
} // namespace webs

/* 获取事件上下文类 */
IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event) {
    switch (event) {
    case IOManager::READ:
        return FdContext::read;
    case IOManager::WRITE:
        return FdContext::write;
    default:
        WEBS_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

/* 重置事件上下文类 */
void IOManager::FdContext::resetContext(EventContext &ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

/* 设置触发事件 */
/* 校验socket_fd事件 --> 重新设置socket_fd事件 --> 获取事件的上下文并执行 -->重置调度器 */
void IOManager::FdContext::triggerEvent(Event event) {
    // 外部已经加锁了
    WEBS_ASSERT(events & event);
    events = (Event)(events & (~event));
    EventContext &ctx = getContext(event);
    if (ctx.cb) {
        // 这里使用的是function对象的地址，ctx.cb在执行FiberAndThread(std::function<void()>*, int)构造函数的时候会被swap为nullptr
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr; // 在添加事件的时候会重新赋一个调度器
    return;
}
/* 构造函数；线程数量、是否将调用线程包含进去、调度器的名称 */
/* 创建epoll_fd  --> 设置m_ticklefd  --> 设置监听事件  --> 注册事件 --> 设置文件描述符容器的属性 -->启动IO管理器 */
IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) :
    Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    WEBS_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    WEBS_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    WEBS_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    WEBS_ASSERT(!rt);

    contextResize(32);

    start();
}

/* 停止调度器 --> 释放文件描述符和sockefd类的动态对象 */
IOManager::~IOManager() {
    stop();

    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i != m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete (m_fdContexts[i]);
        }
    }
}

/* 重置socket事件上下文容器的大小 */
void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size); //只是改变容器的大小，并不进行初始化
    for (size_t i = 0; i != size; ++i) {
        if (!m_fdContexts[i]) { // 上下文不存在才会被重置
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

/* 添加事件；成功返回0，失败返回-1 或者 程序中断 */
/* 获取fd对应的socket_fd上下文 --> 校验socket_fd是否已存在当前event  --> 添加epoll监听事件 --> 设置socket_fd属性 */
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext *fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5); // 当前文件描述符容器不够，先扩容再添加
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock1(fd_ctx->mutex);
    if (WEBS_UNLIKELY(fd_ctx->events & event)) { // 添加相同事件意味着至少有两个线程在操作同一个文件描述符 ---是危险性行为
        // ()强转的优先级 低于 ->
        WEBS_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                                 << "event = " << (EPOLL_EVENTS)event
                                 << "fd_ctx->event = " << (EPOLL_EVENTS)fd_ctx->events;
        WEBS_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD; // 查看是添加还是修改
    epoll_event events;
    events.events = EPOLLET | fd_ctx->events | event;
    events.data.ptr = fd_ctx; // 存放在数据字段，意味着可以在回调的时候通过该字段判断从哪一个ft_ctx触发的

    int rt = epoll_ctl(m_epfd, op, fd, &events);
    if (rt) {
        WEBS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                 << (EpollCtop)op << ", " << fd << ", " << (EPOLL_EVENTS)events.events << ", "
                                 << rt
                                 << "( " << errno << ") (" << strerror(errno) << ") fd_ctx->events = "
                                 << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    // event可读可写，但是不能重复添加读或者重复添加写
    FdContext::EventContext &ctx = fd_ctx->getContext(event);
    // 拿出来的event_ctx应该是空的，如果不为空，说明有事件
    WEBS_ASSERT(!ctx.scheduler && !ctx.fiber && !ctx.cb);
    ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        ctx.cb.swap(cb);
    } else {
        ctx.fiber = Fiber::GetThis(); // 这里的协程应该是正在执行中的
        WEBS_ASSERT2(ctx.fiber->getState() == Fiber::EXEC, "state = " << ctx.fiber->getState());
    }
    return 0;
}

/* 删除事件 */
/* 获取fd对应的socket_fd上下文 --> 校验socket_fd是否已存在当前event  --> 删除event后，重新添加epoll监听事件 --> 设置socket_fd属性 */
bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        WEBS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                 << (EpollCtop)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "): "
                                 << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    fd_ctx->resetContext(fd_ctx->getContext(event));

    return true;
}

/* 取消事件 */
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        WEBS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                 << (EpollCtop)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "): "
                                 << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event); // 触发事件
    --m_pendingEventCount;
    return true;
}

/* 取消所有事件 */
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events)) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        WEBS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                 << (EpollCtop)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "): "
                                 << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ); // 触发事件
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE); // 触发事件
        --m_pendingEventCount;
    }
    WEBS_ASSERT(fd_ctx->events == 0);
    return true;
}

/* 返回当前的IOManager */
IOManager *IOManager::GetThis() {
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

/* 返回是否可以停止 */
bool IOManager::stopping(uint64_t &timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

/* 当有消息来临的时候就发消息 ---> 唤醒线程执行任务 */
void IOManager::tickle() {
    if (!hasIdleThreads()) {
        return;
    }
    // WEBS_LOG_INFO(g_logger) << "IOManager::tickle ";
    // 向管道中发送一个消息，通知线程执行任务
    int rt = write(m_tickleFds[1], "T", 1);
    WEBS_ASSERT(rt == 1);
}

/* 协程无任务可以调度时，执行idle协程 */
/* 设置智能指针动态数组 ---> 设置epoll_wait参数 ---> 执行定时器回调函数 ---> 
处理epoll_wait事件：1、唤醒线程的事件 --> 读取文件描述符的内容；
2、其他事件：重新设置epollfd的监听事件 --> 触发读写事件  ---> 切换执行线程 */
void IOManager::idle() {
    // 设置epoll事件数组
    WEBS_LOG_DEBUG(g_logger) << " idle";
    const uint64_t MAX_EVENTS = 256;
    epoll_event *events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr) { delete[] ptr; });

    while (true) {
        // 是否已经停止
        uint64_t next_timeout = 0;
        if (WEBS_UNLIKELY(stopping(next_timeout))) {
            WEBS_LOG_INFO(g_logger) << "name = " << getName()
                                    << " idle stopping exit";
            break;
        }

        // 设置超时等待的epoll_wait函数
        int rt = 0;
        do {
            // 最大超时时间
            static const int MAX_TIMEOUT = 3000;
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            WEBS_LOG_DEBUG(g_logger) << " epoll_wait time " << next_timeout;

            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            if (rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while (true);

        WEBS_LOG_DEBUG(g_logger) << " epoll_wait rt " << rt;

        // 处理定时器的回调函数；如果有的话
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);

        if (!cbs.empty()) {
            // for (auto &i : cbs) {
            //     schedule(i);
            // }
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        // 处理事件
        for (int i = 0; i < rt; ++i) {
            epoll_event &event = events[i];
            // 检查事件是否是唤醒线程的事件
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t buf[256];
                while (read(m_tickleFds[0], buf, sizeof(buf)) > 0)
                    ;
                continue;
            }

            // 确定事件
            FdContext *ctx = (FdContext *)event.data.ptr;
            FdContext::MutexType::Lock lock(ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & ctx->events; // 将监听事件设置为socket的读或者写事件
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            // 校验事件
            if ((ctx->events & real_events) == NONE) {
                continue;
            }

            // 重新设置监听事件
            Event new_event = (Event)(ctx->events & (~event.events));
            int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | new_event;
            int res = epoll_ctl(m_epfd, op, ctx->fd, &event);
            if (res) {
                WEBS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                         << (EpollCtop)op << ", " << ctx->fd << ", "
                                         << (EPOLL_EVENTS)event.events << "): " << res
                                         << "( " << errno << strerror(errno) << ")";
                continue;
            }

            // 触发事件
            if (real_events == READ) {
                ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_events == WRITE) {
                ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        // 切换其他上下文执行，不能直接使用swapout
        Fiber::ptr fiber = Fiber::GetThis();
        auto raw = fiber.get();
        fiber.reset();
        raw->swapOut();
    }
}

/* 当有新的定时器插入到定时器的首部,执行该函数 */
void IOManager::onTimerInsertedAtFront() {
    tickle();
}

} // namespace webs