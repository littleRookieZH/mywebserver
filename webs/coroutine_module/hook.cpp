#include "hook.h"
#include "fd_manager.h"
#include "../log_module/log.h"
#include "../config_module/config.h"
#include "../IO_module/iomanager.h"
#include "../util_module/macro.h"

#include <dlfcn.h>
#include <functional>
#include <stdarg.h>

webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");
namespace webs {

static webs::ConfigVar<int>::ptr g_tcp_connect_timeout = webs::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

//  writev_f = (writev_fun)dlsym(((void *) -1l), "writev");
// writev_f = (writev_fun)dlsym(((void *)-1l), "writev");
void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    } else {
        is_inited = true;
    }
// dlsym 动态库中取函数的方法； ## 是一个连接符；从系统库中取出对应的函数，并将其转为需要的函数方式 (...)，赋值给name ## _fun
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;

/* 想要在main函数执行之前初始化hook --> 可以利用数据初始化的先后顺序 --> 全局变量在main之前进行初始化 */
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int &old_value, const int &new_value) { WEBS_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                                                                                                    << old_value << " to " << new_value; 
                                                                                                                    s_connect_timeout = new_value; });
    }
};

// 自动初始化
static _HookIniter s_hook_initer;

/* 设置线程的hook状态 */
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

/* 返回hook状态 */
bool is_hook_enable() {
    return t_hook_enable;
}

} // namespace webs

struct timer_info {
    int cancelled = 0;
};

/* 文件描述符、可调用对象、传入需要hook的函数名、
IO_Event、超时时间的类型( SO_RCVTIMEO or SO_SNDTIMEO )，以及执行hook函数所需参数
返回-1表示错误，其他的返回值与hook的函数一致
*/
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_name, uint32_t event, int timeout_so, Args &&... args) {
    if (!webs::t_hook_enable) { // 非hook
        return fun(fd, std::forward<Args>(args)...);
    }
    // 获取FdManger保存的fd
    webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(fd);
    if (!ctx) { // 如果不存在
        return fun(fd, std::forward<Args>(args)...);
    }
    if (ctx->isClose()) {
        errno = EBADF; // 文件描述符错误
        return -1;
    }
    if (!ctx->isSocket() || ctx->getUserNonblock()) { // 不是socket 或者 用户设置了堵塞
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    // 添加条件定时器
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // fd为socket，且是非堵塞 --> 直接执行fun，如果出错，errno = EINTR --> 继续尝试执行fun -->
    // 如果errno = EAGAIN -->设置条件定时器 和 事件来帮助我们定时唤醒或者触发事件唤醒，继续执行fun函数尝试再次读取
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR) { // 如果读取中断，继续读取
        fun(fd, std::forward<Args>(args)...);
    }
    // EAGAIN 错误通常表示非阻塞操作正在进行中，但是当前没有数据可供读取或者没有空间可供写入，需要等待一段时间后再次尝试。
    if (n == -1 && errno == EAGAIN) {
        webs::IOManager *iom = webs::IOManager::GetThis();
        webs::Timer::ptr timer;
        if (to != (uint64_t)-1) {
            std::weak_ptr<timer_info> winfo(tinfo);
            timer = iom->addConditionTimer(
                to, [winfo, iom, fd, event]() {
                    auto it = winfo.lock();
                    if (!it || it->cancelled) { // 条件不存在 或者 已经设置了 ETIMEDOUT 错误
                        return;
                    }
                    it->cancelled = ETIMEDOUT;
                    iom->delEvent(fd, (webs::IOManager::Event)event); // 取消事件
                },
                winfo);
        }
        // 添加事件
        int rt = iom->addEvent(fd, (webs::IOManager::Event)event);
        if (WEBS_UNLIKELY(rt)) {
            // 输出错误信息
            WEBS_LOG_ERROR(g_logger) << hook_name << " addEvent(" << fd << " , " << event << ") fail";
            // 取消定时器
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {
            webs::Fiber::YieldToHold(); // 让出自己的执行时间
            // 如果切回来表示有数据来了
            // 从两个地方切回来：1、执行定时器任务之后 2、addEvent
            if (timer) { // addEvent
                timer->cancel();
            }
            if (tinfo->cancelled) { // 执行定时器任务之后
                errno = tinfo->cancelled;
                return -1;
            }
            // 当前协程被唤醒，继续尝试读取数据
            goto retry;
        }
    }
    return n; // 可能读到数据，也可能出了其他错误
}

extern "C" {
// 创建一个类型、一个变量
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

/* 添加定时器；如果设置了hook，则使用定时器代替 sleep */
unsigned int sleep(unsigned int second) {
    if (!webs::t_hook_enable) {
        return sleep_f(second);
    }
    webs::IOManager *iom = webs::IOManager::GetThis();
    webs::Fiber::ptr fiber = webs::Fiber::GetThis();
    // 定时器的任务是：到时间之后，执行当前协程的上下文
    // webs::Scheduler:: 表示成员函数所属的类作用域
    iom->addTimer(second * 1000, std::bind((void (webs::Scheduler::*)(webs::Fiber::ptr fc, int thread)) & webs::IOManager::schedule, iom, fiber, -1));
    webs::Fiber::YieldToHold();
    return 0;
}

/* 添加定时器；如果设置了hook，则使用定时器代替 usleep */
int usleep(useconds_t usec) {
    if (!webs::t_hook_enable) {
        return usleep_f(usec);
    }

    webs::IOManager *iom = webs::IOManager::GetThis();
    webs::Fiber::ptr fiber = webs::Fiber::GetThis();
    iom->addTimer(usec / 1000, std::bind((void (webs::Scheduler::*)(webs::Fiber::ptr fc, int thread)) & webs::IOManager::schedule, iom, fiber, -1));
    webs::Fiber::YieldToHold();
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!webs::t_hook_enable) {
        return nanosleep_f(req, rem);
    }
    webs::IOManager *iom = webs::IOManager::GetThis();
    webs::Fiber::ptr fiber = webs::Fiber::GetThis();
    uint64_t ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    iom->addTimer(ms, std::bind((void (webs::Scheduler::*)(webs::Fiber::ptr fc, int thread)) & webs::IOManager::schedule, iom, fiber, -1));
    webs::Fiber::YieldToHold();
}

/* 如果设置了hook，则还需要将fd加入到Fdctx中 */
int socket(int domain, int type, int protocol) {
    if (!webs::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }

    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }
    webs::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

/* hook? fdmanager的fd? issocket? usernonblock? errno == EINPROGRESS?  设置条件定时器、添加事件 获取错误值*/
int conntect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
    if (!webs::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose()) {
        return -1;
    }
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }
    int rt = connect_f(fd, addr, addrlen);
    if (rt == 0) {
        return 0;
    } else if (rt != -1 || errno != EINPROGRESS) {
        return rt;
    }

    // rt == -1  或者 errno == EINPROGRESS  ---> 添加定时器和事件
    webs::IOManager *iom = webs::IOManager::GetThis();
    // 设置条件定时器
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);
    webs::Timer::ptr timer;
    if (timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(
            timeout_ms, [winfo, iom, fd]() {
                auto it = winfo.lock();
                if (!it || it->cancelled) {
                    return;
                }
                it->cancelled = ETIMEDOUT;
                iom->delEvent(fd, webs::IOManager::Event::WRITE);
            },
            winfo);
    }
    int rt = iom->addEvent(fd, webs::IOManager::Event::WRITE);
    if (rt == 0) {
        webs::Fiber::YieldToHold();
        if (timer) {
            timer->cancel();
        }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) {
            timer->cancel();
        }
        WEBS_LOG_ERROR(g_logger) << "connect addEvent( " << fd << ", WRITE) error";
    }

    // 如果有错误的话，输出一下
    int error = 0;
    socklen_t len = sizeof(int);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
        return -1;
    }
    if (error == 0) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

/* 返回值：0表示连接成功；-1表示连接失败 */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return conntect_with_timeout(sockfd, addr, addrlen, webs::s_connect_timeout);
}

/* 使用do_io 监听事件：读事件、超时时间：SO_RCVTIMEO
返回值：-1表示错误，非负表示成功，值对应一个新的套接字文件描述符
 */
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(sockfd, accept_f, "accept", webs::IOManager::Event::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        webs::FdMgr::GetInstance()->get(fd, true); // 如果成功，将其添加到 fdmanager
    }
    return fd;
}

/* 使用do_io 监听事件：读事件、超时时间：SO_RCVTIMEO
返回值：-1表示错误，非负数表示读取的字符数
 */
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", webs::IOManager::Event::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", webs::IOManager::Event::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", webs::IOManager::Event::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", webs::IOManager::Event::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", webs::IOManager::Event::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", webs::IOManager::Event::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", webs::IOManager::Event::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", webs::IOManager::Event::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", webs::IOManager::Event::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", webs::IOManager::Event::WRITE, SO_SNDTIMEO, msg, flags);
}

/* hook? fdmanager保存了fd? 如果保存了触发所有事件，并从fdmanager删除fd(实际上是对fd reset) */
int close(int fd) {
    if (!webs::t_hook_enable) {
        return close_f(fd);
    }
    webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(fd);
    if (ctx) {                                             // 如果fdmanager保存了 fd
        webs::IOManager *iom = webs::IOManager::GetThis(); // 这里在什么情况下为nullptr
        if (iom) {
            iom->cancelAll(fd); // 触发所有事件
        }
        webs::FdMgr::GetInstance()->del(fd); // 删除保存的fd
    }
    return close_f(fd);
}

/**
 * 获取可变参数列表
 * F_SETFL:获取int类型可变参数arg fd 是否在 fgmanager中？close? issocket?  设置ctx->setusernonblock、根据sysnoblock将arg设置为堵塞或者非堵塞
 * F_GETFL:fd 是否在 fgmanager中？close? issocket?  根据usrnonblock决定arg是否添加O_NONBLOCK
 * F_DUPFD、F_DUPFD_CLOEXEC、F_SETFD、F_SETOWN、F_SETSIG、F_SETLEASE、F_NOTIFY 获取int类型可变参数arg，执行fcntl(fd, cmd, arg)
 * F_GETFD、F_GETOWN、F_GETSIG、F_GETLEASE 直接执行 fcntl(fd, cmd)
 * F_SETLK、F_SETLKW、F_GETLK 获取struct flock* 类型(与文件锁定相关)可变参数arg，执行fcntl(fd, cmd, arg)
 * F_GETOWN_EX、F_SETOWN_EX 获取struct f_owner_exlock* 类型(用于文件描述符的所有者信息和锁定（locking）)可变参数arg，执行fcntl(fd, cmd, arg)
*/
int fcntl(int fd, int cmd, ... /* arg */) {
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
    case F_SETFL: {
        int arg = va_arg(va, int);
        va_end(va);
        webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return fcntl_f(fd, cmd, arg); // 普通fd
        }
        // 如果是一个socket
        ctx->setUserNonblock(arg & O_NONBLOCK);
        if (ctx->getSysNonblock()) { // arg的结果取决于系统设置
            arg |= O_NONBLOCK;
        } else {
            arg &= ~O_NONBLOCK;
        }
        return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFL: {
        va_end(va);
        int arg = fcntl_f(fd, cmd);
        webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return arg;
        }
        // 是socket还需要决定arg是否存在O_NONBLOCK
        if (ctx->getUserNonblock()) {
            return arg | O_NONBLOCK;
        } else {
            return arg & ~O_NONBLOCK;
        }
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
#ifdef F_SETPIPE_SZ
    case F_SETPIPE_SZ:
#endif
    {
        int arg = va_arg(va, int);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
#ifdef F_GETPIPE_SZ
    case F_GETPIPE_SZ:
#endif
    {
        va_end(va);
        return fcntl_f(fd, cmd);
    }break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:{
        struct flock* arg = va_arg(va, struct flock*);// 为什么加了()就不对了
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    }break;
    case F_GETOWN_EX:
    case F_SETOWN_EX:{
        struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    }break;
    default:
        va_end(va);
        return fcntl_f(fd, cmd);
    }
}

/* 读取可变参数，类型为void*；如果request为 FIONBIO，如果是socket还需要设置usr为非堵塞 */
int ioctl(int fd, unsigned long request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(request == FIONBIO){
        bool usr_nonblock = !!*(int*)arg;
        webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket()){
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(usr_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname,
               void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

/* 非hook直接执行；hook需要设置超时时间 */
/* hook？ level == SOL_SOCKET? --> optname = RCVTIMEO || SNDTIMEO? --> 设置超时时间 */
int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen) {
    if(!webs::t_hook_enable){
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET){        
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO){
            webs::FdCtx::ptr ctx = webs::FdMgr::GetInstance()->get(sockfd);
            if(ctx){
                const struct timeval* timeout = (const struct timeval*)optval;
                ctx->setTimeout(optname, timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}

// int main() {
//     return 0;
// }