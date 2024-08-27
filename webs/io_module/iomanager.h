#ifndef __WEBS_IOMANAGER_H__
#define __WEBS_IOMANAGER_H__
#include "../coroutine_module/scheduler.h"
#include "../log_module/log.h"
#include "../io_module/timer.h"

namespace webs {
// 每一个类的继承权限都是单独的；public不可省
class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    enum Event {
        // 无事件
        NONE = 0x0,
        // 读事件(EPOLLIN)
        READ = 0x1,
        // 写事件(EPOLLOUT)
        WRITE = 0x04
    };

private:
    // Socket事件上下文类
    struct FdContext {
        typedef Mutex MutexType;
        // 事件上下文类
        struct EventContext {
            // 事件执行的调度器
            Scheduler *scheduler = nullptr;
            // 事件协程
            Fiber::ptr fiber;
            // 事件的回调函数
            std::function<void()> cb;
        };

        /* 获取事件上下文类 */
        EventContext &getContext(Event event);

        /* 重置事件上下文类 */
        void resetContext(EventContext &ctx);

        /* 设置触发事件 */
        void triggerEvent(Event event);

        // 读事件上下文
        EventContext read;
        // 写事件上下文
        EventContext write;
        // 事件关联的文件描述符
        int fd = 0;
        // 当前的事件
        Event events = NONE;
        MutexType mutex;
    };

public:
    /* 构造函数；线程数量、是否将调用线程包含进去、调度器的名称 */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");

    ~IOManager();

    /* 添加事件 */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /* 删除事件 */
    bool delEvent(int fd, Event event);

    /* 取消事件 */
    bool cancelEvent(int fd, Event event);

    /* 取消所有事件 */
    bool cancelAll(int fd);

    /* 返回当前的IOManager */
    static IOManager *GetThis();

protected:
    /* 返回是否可以停止 */
    bool stopping() override;

    /* 协程无任务可以调度时，执行idle协程 */
    void idle() override;

    /* 通知协程调度器有任务了；可以自定义调度器的执行方式(如何调度) */
    void tickle() override;

    /* 当有新的定时器插入到定时器的首部,执行该函数 */
    void onTimerInsertedAtFront() override;

    /* 重置socket事件上下文容器的大小 */
    void contextResize(size_t size);

    /* 判断是否可以停止 */
    bool stopping(uint64_t &timeout);

private:
    // epoll的文件描述符
    int m_epfd = 0;
    // pipe 文件描述符
    int m_tickleFds[2];
    // 用于创建原子对象；当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    // 读写锁
    RWMutexType m_mutex;
    // socket事件上下文的容器
    std::vector<FdContext *> m_fdContexts;
};
} // namespace webs

#endif