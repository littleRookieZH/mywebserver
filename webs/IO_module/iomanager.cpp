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
}

/* 输出epoll_events */
std::ostream &operator<<(std::ostream &os, EPOLL_EVENTS events) {
    if (!events) {
        os << "0";
    }
    bool first = true;
#define XX(E)          \
    if (E == events) { \
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
IOManager::FdContext::EventContext &IOManager::FdContext::getContext(Event event) {
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
        ctx.scheduler->schedule(ctx.fiber);
    }
    ctx.scheduler = nullptr; // 在添加事件的时候会重新赋一个调度器
    return;
}
/* 构造函数；线程数量、是否将调用线程包含进去、调度器的名称 */
/* 创建epoll_fd  --> 设置m_ticklefd  --> 设置监听事件  --> 注册事件 --> 设置文件描述符容器的属性 -->启动IO管理器 */
IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) :
    Scheduler(threads, use_caller, name) {
    int epoll_fd = epoll_create(5000);
    WEBS_ASSERT(epoll_fd > 0);

    int rt = pipe(m_tickleFds);
    WEBS_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    WEBS_ASSERT(!rt);

    rt = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
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

/* 添加事件 */
int IOManager::addEvent(int fd, Event event, std::function<void()> cb = nullptr) {
}
} // namespace webs