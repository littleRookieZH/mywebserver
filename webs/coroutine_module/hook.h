/**
 * hook函数的封装
*/
#ifndef __WEBS_HOOK__
#define __WEBS_HOOK__

#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>

namespace webs {

/* 设置线程的hook状态 */
void set_hook_enable(bool flag);

/* 当前线程是否hook；决定是否需要hook，支持线程级别的控制 */
bool is_hook_enble();
} // namespace webs

/**
 * extern "C" {} 是 C++ 中的一种语法，用于指定特定的函数或变量使用 C 语言的命名约定进行编译和链接。
 * */
extern "C" {
// sleep
// 用于使当前线程休眠指定的秒级别的时间。
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

// 用于使当前线程休眠指定的微秒级别的时间。
typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

// 用于使当前线程休眠指定的纳秒级别的时间。
typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

// socket
typedef int (*socket_fun)(int damain, int type, int protocol);
extern socket_fun socket_f;

/* 客户端  连接服务器 */
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

/* 服务器创建新连接 */
typedef int (*accept_fun)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

// read
/* 针对文件（通用读方式：万物皆文件） */
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

/* 保存数据的结构类似与链表，链表节点属性：内存地址、长度 */
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

// recv
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

// write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

// send
typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags,
                              const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
extern fcntl_fun fcntl_f;

// 用于设备控制的系统调用，通常用于对设备进行各种操作，如设置参数、查询状态等。
typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
extern ioctl_fun ioctl_f;

// 用于获取套接字选项的系统调用，通常用于查询套接字的配置信息和状态。
typedef int (*getsockopt_fun)(int sockfd, int level, int optname,
                              void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                              const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms);
}
#endif