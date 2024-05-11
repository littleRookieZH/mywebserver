#include "socket.h"
#include "../log_module/log.h"
#include "../coroutine_module/fd_manager.h"
#include "../util_module/macro.h"

namespace webs {
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

/* 协议簇、socket类型(流式\数据包式)、具体的协议 */
Socket::Socket(int family, int type, int protocol) :
    m_sock(-1),
    m_family(family),
    m_type(type),
    m_protocol(m_protocol),
    m_isConnected(false) {
}

/* 创建满足地址类型的 TCP地址 */
Socket::ptr Socket::CreateTCP(webs::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

/* 创建满足地址类型的 UDP地址 */
Socket::ptr Socket::CreateUDP(webs::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();
    return sock;
}

/* 创建IPv4的TCP地址 */
Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

/* 创建IPv4的UDP地址 */
Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    return sock;
}

/* 创建IPv6的TCP地址 */
Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

/* 创建IPv6的UPD地址 */
Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    return sock;
}

/* 创建UNIX的TCP地址 */
Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

/* 创建UNIX的UDP地址 */
Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

Socket::~Socket() {
    close();
}

/* 获取发送超时时间(毫秒)；从fdmrg中获取socketfd的超时时间属性 */
int64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

/* v: 毫秒数；使用setOption函数调用setsockopt库函数 */
void Socket::setSendTimeout(uint64_t v) {
    struct timeval tv { // 秒数，微秒数 -- 列表初始化，赋初值
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/* 获取接受超时时间(毫秒) */
uint64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

/* 设置接收超时时间(毫秒) */
void Socket::setRecvTimeout(uint64_t v) {
    struct timeval tv { // 秒数，微秒数
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

/* 获取sockopt; @see getsockopt() */
bool Socket::getOption(int level, int optname, void *optval, socklen_t *optlen) {
    int rt = getsockopt(m_sock, level, optname, optval, (socklen_t *)optlen);
    if (rt) {
        WEBS_LOG_DEBUG(g_logger) << "getOption sock = " << m_sock
                                 << " level = " << level << " optname = " << optname
                                 << " optval = " << optval << " errno = " << errno << " strerr = " << strerror(errno);
        return false;
    }
    return true;
}

/* 设置sockopt; @see setsockopt() */
bool Socket::setOption(int level, int optname, const void *optval, socklen_t optlen) {
    int rt = setsockopt(m_sock, level, optname, optval, optlen);
    if (rt) {
        WEBS_LOG_DEBUG(g_logger) << "setOption sock = " << m_sock
                                 << " level = " << level << " optname = " << optname
                                 << " optval = " << optval << " errno = " << errno << " strerr = " << strerror(errno);
        return false;
    }
    return true;
}

/* socketfd是否有效 --> sock的协议与地址协议是否一致 -->
    是否是unix域地址，如果是需要检验是否地址已经被绑定(直接connect--》成功说明已绑定) -->  连接失败，先删除路径(防止未释放) 
 ---> 将socketfd 与 地址 */
bool Socket::bind(webs::Address::ptr addr) {
    if (!isVaild()) { // m_sock 无效
        newSock();    // 创建socketfd
        if (WEBS_UNLIKELY(!isVaild())) {
            return false;
        }
    }

    if (WEBS_UNLIKELY(m_family != addr->getFamily())) { // sock协议与地址协议不一致
        WEBS_LOG_DEBUG(g_logger) << "bind sock.family(" << m_family
                                 << "), addr family(" << addr->getFamily() << ") not equal. addr = " << addr->toString();
        return false;
    }

    webs::UinxAddress::ptr unixaddr = std::dynamic_pointer_cast<UinxAddress>(addr);
    if (unixaddr) { // 如果是unix
        Socket::ptr unixsock = CreateUnixTCPSocket();
        if (unixsock->connect(unixaddr)) { // 连接成功，说明当前地址已经被监听socket绑定
            return false;
        } else {
            webs::FSUtil::Unlink(unixaddr->getPath(), true); // 删除可能存在的文件，防止上一次连接未释放
        }
    }
    if (::bind(m_sock, addr->getAddr(), addr->getAddrlen())) { // 尝试绑定
        WEBS_LOG_DEBUG(g_logger) << "bind error errno = " << errno << " strerr = " << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
}

/* 创建socket */
void Socket::newSock() {
}

} // namespace webs

int main() {
    return 0;
}