#include "socket.h"
#include "../log_module/log.h"
#include "../coroutine_module/fd_manager.h"
#include "../util_module/macro.h"
#include "../coroutine_module/hook.h"
#include "../IO_module/iomanager.h"

#include <netinet/tcp.h>
#include <openssl/err.h>

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
                                 << " optval = " << optval << " errno = " << errno << " errstr = " << strerror(errno);
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
                                 << " optval = " << optval << " errno = " << errno << " errstr = " << strerror(errno);
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

    webs::UnixAddress::ptr unixaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if (unixaddr) { // 如果是unix
        Socket::ptr unixsock = CreateUnixTCPSocket();
        if (unixsock->connect(unixaddr)) { // 连接成功，说明当前地址已经被监听socket绑定
            return false;
        } else {
            webs::FSUtil::Unlink(unixaddr->getPath(), true); // 删除可能存在的文件，防止上一次连接未释放
        }
    }
    if (::bind(m_sock, addr->getAddr(), addr->getAddrlen())) { // 尝试绑定
        WEBS_LOG_DEBUG(g_logger) << "bind error errno = " << errno << " errstr = " << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

/* 检查m_sock是否有效  --  检查socketfd的协议与所连接的的地址协议是否相同、
  --  如果没有设置超时时间，直接connect 如果设置了超时时间 使用超时连接 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;
    if (!isVaild()) {
        newSock();
        if (WEBS_UNLIKELY(!isVaild())) {
            return false;
        }
    }

    if (WEBS_UNLIKELY(addr->getFamily() != m_family)) {
        WEBS_LOG_DEBUG(g_logger) << "connect sock.family(" << m_family
                                 << ") addr.family(" << addr->getFamily()
                                 << ") not equal, addr = " << addr->toString();
        return false;
    }

    if (timeout_ms == (uint64_t)-1) {
        if (::connect(m_sock, addr->getAddr(), addr->getAddrlen())) {
            WEBS_LOG_DEBUG(g_logger) << "sock = " << m_sock << "connect (" << addr->toString()
                                     << ") error errno = " << errno << "errstr = " << strerror(errno);
            close();
            return false;
        }
    } else {
        if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrlen(), timeout_ms)) {
            WEBS_LOG_DEBUG(g_logger) << "sock = " << m_sock << "connect(" << addr->toString()
                                     << ") error errno = " << errno << "errstr = " << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

/* 检查远端地址 --- 重置本地地址 --- 连接远端地址 */
bool Socket::reconnect(uint64_t timeout_ms) {
    if (!m_remoteAddress) {
        WEBS_LOG_DEBUG(g_logger) << "reconnect m_remoteAddress is null";
        return false;
    }
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}

/* 关闭fd，但不从fdmrg中删除fd */
bool Socket::close() {
    if (!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if (m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
        return true;
    }
    return false;
}

/* 校验是否有效 -- 连接失败输出日志 */
bool Socket::listen(int backlog) {
    if (!isVaild()) {
        WEBS_LOG_DEBUG(g_logger) << "listen error sock = -1";
        return false;
    }

    if (::listen(m_sock, backlog)) {
        WEBS_LOG_DEBUG(g_logger) << "listen error errno = " << errno << "errstr = " << strerror(errno);
        return false;
    }
    return true;
}

/* 创建连接sock --- 使用accpt创建连接sockfd --- 对sock初始化 */
Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int sockfd = ::accept(m_sock, nullptr, nullptr); // 不需要指定客户端地址以及地址长度
    if (sockfd == -1) {
        WEBS_LOG_DEBUG(g_logger) << "accept(" << m_sock
                                 << ") errno = " << errno << "errstr = " << strerror(errno);
        return nullptr;
    }
    if (sock->init(sockfd)) { // 初始化sock
        return sock;
    }
    return nullptr;
}

/* 向FdMgr添加sockfd --- 初始化sock：设置sock选项，设置sock对象的一些属性值 */
bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if (ctx && !ctx->isClose() && ctx->isSocket()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

/* 设置一些sock选项 --- sockfd的本机ip和端口重用 --- 设置关闭延迟发送 */
void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

/* 创建socket，成功则设置sockopt，失败输出日志 */
void Socket::newSock() {
    int sockfd = ::socket(m_family, m_type, m_protocol);
    if (sockfd != -1) {
        initSock();
    } else {
        WEBS_LOG_DEBUG(g_logger) << "socket(" << m_family << ", "
                                 << m_type << ", " << m_protocol << ") errno = "
                                 << errno << "errstr = " << strerror(errno);
    }
}

/* 发送length个bit到m_sock；校验是否已经连接好了 */
int Socket::send(const void *buf, size_t length, int flags) {
    if (m_isConnected) {
        return ::send(m_sock, buf, length, flags);
    }
    return -1;
}

/* 使用sendmsg发送数据 */
int Socket::send(const iovec *buf, size_t length, int flags) {
    if (m_isConnected) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/* 发送前进行校验是否连接；指定发送的目的地 */
int Socket::sendTo(const void *buf, size_t length, const Address::ptr to, int flags) {
    if (m_isConnected) {
        return ::sendto(m_sock, buf, length, flags, to->getAddr(), to->getAddrlen());
    }
    return -1;
}

/* 发送前进行校验是否连接 */
int Socket::sendTo(const iovec *buf, size_t length, const Address::ptr to, int flags) {
    if (m_isConnected) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();       // Address to send to/receive from.
        msg.msg_namelen = to->getAddrlen(); // Length of address data.
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/* 接受前校验是否连接 */
int Socket::recv(void *buf, size_t length, int flags) {
    if (m_isConnected) {
        return ::recv(m_sock, buf, length, flags);
    }
    return -1;
}

/* 接受前校验是否连接 */
int Socket::recv(iovec *buf, size_t length, int flags) {
    if (m_isConnected) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/* 接收数据，并获取远端地址的信息 */
int Socket::recvFrom(void *buf, size_t length, const Address::ptr from, int flags) {
    if (m_isConnected) {
        socklen_t val = from->getAddrlen();
        return ::recvfrom(m_sock, buf, length, flags, from->getAddr(), &val);
    }
    return -1;
}

int Socket::recvFrom(iovec *buf, size_t length, const Address::ptr from, int flags) {
    if (m_isConnected) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();       // Address to send to/receive from.
        msg.msg_namelen = from->getAddrlen(); // Length of address data.
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/* 输出实例的信息 */
std::ostream &Socket::dump(std::ostream &os) const {
    os << "[Socket sock = " << m_sock
       << ", is_connected" << m_isConnected
       << ", family = " << m_family
       << ", type = " << m_type
       << ", protocol = " << m_protocol;
    if (m_localAddress) {
        os << ", local_address = " << m_localAddress->toString();
    }
    if (m_remoteAddress) {
        os << ", remote_address = " << m_remoteAddress->toString();
    }
    return os << "]";
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

/* 根据类型创建Address -- 使用getpeername获取远端地址 -- 如果类型是unix还需要设置地址长度(unix地址是字符数组) */
Address::ptr Socket::getRemoteAddress() {
    if (m_remoteAddress) {
        return m_remoteAddress;
    }
    Address::ptr addr;
    if (m_family == AF_INET) {
        addr.reset(new IPv4Address());
    } else if (m_family == AF_INET6) {
        addr.reset(new IPv6Address());
    } else if (m_family == AF_UNIX) {
        addr.reset(new UnixAddress());
    } else {
        addr.reset(new UnknownAddress(m_family));
    }
    socklen_t len = addr->getAddrlen();
    if (getpeername(m_sock, addr->getAddr(), &len)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr unixaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
        unixaddr->setAddrlen(len);
    }
    m_remoteAddress = addr;
    return m_remoteAddress;
}

/* 根据类型创建地址 -- 使用getsockname函数获取本地地址(IP + port) -- 如果是unix，设置地址的长度 */
Address::ptr Socket::getLocalAddress() {
    if (m_localAddress) {
        return m_localAddress;
    }
    Address::ptr addr;
    switch (m_family) {
    case AF_INET:
        addr.reset(new IPv4Address());
        break;
    case AF_INET6:
        addr.reset(new IPv6Address());
        break;
    case AF_UNIX:
        addr.reset(new UnixAddress);
        break;
    default:
        addr.reset(new UnknownAddress(m_family));
        break;
    }
    socklen_t len = addr->getAddrlen();
    if (getsockname(m_sock, addr->getAddr(), &len)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr unixaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
        unixaddr->setAddrlen(len);
    }
    m_localAddress = addr;
    return m_localAddress;
}

int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    // 成功返回true，失败返回false
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

/* 检验fd */
bool Socket::cancelRead() {
    if (m_sock == -1) {
        return false;
    }
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return false;
    }
    return IOManager::GetThis()->cancelEvent(m_sock, webs::IOManager::READ);
}

bool Socket::cancelWrite() {
    if (m_sock == -1) {
        return false;
    }
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return false;
    }
    return webs::IOManager::GetThis()->cancelEvent(m_sock, webs::IOManager::WRITE);
}

/* 监听sock是通过读事件触发的，取消读事件就无法再接收新连接了 */
bool Socket::cancelAccept() {
    if (m_sock == -1) {
        return false;
    }
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return false;
    }
    return webs::IOManager::GetThis()->cancelEvent(m_sock, webs::IOManager::READ);
}

bool Socket::cancelAll() {
    if (m_sock == -1) {
        return false;
    }
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return false;
    }
    return webs::IOManager::GetThis()->cancelAll(m_sock);
}

namespace { // 匿名命名空间 变量和函数类似于静态变量和函数；作用域是当前文件
struct _SSLInit {
    _SSLInit() {
        SSL_library_init();           // 初始化SSL库
        SSL_load_error_strings();     // 加载SSL错误信息字符串
        OpenSSL_add_all_algorithms(); // 添加openssl的加密算法
    }
};

_SSLInit s_init;
} // namespace

SSLSocket::SSLSocket(int family, int type, int protocol) :
    Socket(family, type, protocol) {
}

SSLSocket::ptr SSLSocket::CreateTCP(Address::ptr address) {
    return SSLSocket::ptr(new SSLSocket(address->getFamily(), TCP, 0));
}

SSLSocket::ptr SSLSocket::CreateTCPSocket() {
    return SSLSocket::ptr(new SSLSocket(AF_INET, TCP, 0));
}

SSLSocket::ptr SSLSocket::CreateTCPSocket6() {
    return SSLSocket::ptr(new SSLSocket(AF_INET6, TCP, 0));
}

bool SSLSocket::bind(webs::Address::ptr addr) {
    return Socket::bind(addr);
}

bool SSLSocket::listen(int backlog) {
    return Socket::listen(backlog);
}

Socket::ptr SSLSocket::accept() {
    return Socket::accept();
}

/**
 * @brief TCP连接先行；SSL握手是在网络连接之上的
 * connect --> 创建SSL_CTX上下文对象 --> 创建SSL结构体对象 --> 进行SSL握手
 * @param addr 
 * @param timeout_ms 
 * @return true 
 * @return false 
 */
bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    bool val = Socket::connect(addr, timeout_ms);
    if (val) {
        // 解析：SSLv23_client_method() - 可以确保最大的兼容性，因为它会自动选择客户端和服务器都支持的最高协议版本。
        // SSL_CTX_free - 用于释放 SSL_CTX 对象及其占用的所有资源。可以防止内存泄露
        // SSL_CTX_new() - 创建一个新的SSL_CTX对象作为框架，以建立启用TLS/SSL的连接。
        m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free); // shared_ptr可以指定 对象 的释放方法
        // SSL_new - 为连接创建一个新的SSL结构
        m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
        // SSL_set_fd - 将一个文件描述符（通常是一个套接字）与一个 SSL 对象关联起来，以便 SSL 对象可以使用该文件描述符进行加密的读写操作。
        SSL_set_fd(m_ssl.get(), m_sock);
        // SSL_connect 是 OpenSSL 库中的一个函数，用于在客户端与服务器之间建立 SSL/TLS 连接。
        val = (SSL_connect(m_ssl.get()) == 1);
    }
    return val;
}

bool SSLSocket::close() {
    return Socket::close();
}

int SSLSocket::send(const void *buf, size_t length, int flags) {
    if (m_ssl) {
        return SSL_write(m_ssl.get(), buf, length); // 加密发送
    }
    return -1;
}

int SSLSocket::send(const iovec *buf, size_t length, int flags) {
    if (!m_ssl) {
        return -1;
    }
    int total = 0;                        // 发送的字节数
    for (size_t i = 0; i < length; ++i) { // 一块一块发送
        int val = SSL_write(m_ssl.get(), buf[i].iov_base, buf[i].iov_len);
        if (val <= 0) { // 发送失败
            return val;
        }
        total += val;
        if (val != (int)buf[i].iov_len) {
            break;
        }
    }
    return total;
}

int SSLSocket::sendTo(const void *buf, size_t length, const Address::ptr to, int flags) {
    WEBS_ASSERT(false); // 禁止使用
    return -1;
}

int SSLSocket::sendTo(const iovec *buf, size_t length, const Address::ptr to, int flags) {
    WEBS_ASSERT(false); // 禁止使用
    return -1;
}

int SSLSocket::recv(void *buf, size_t length, int flags) {
    if (m_ssl) {
        return SSL_read(m_ssl.get(), buf, length);
    }
    return -1;
}

int SSLSocket::recv(iovec *buf, size_t length, int flags) {
    if (!m_ssl) {
        return -1;
    }
    int total = 0;
    for (size_t i = 0; i < length; ++i) {
        int val = SSL_read(m_ssl.get(), buf[i].iov_base, buf[i].iov_len);
        if (val <= 0) {
            return val;
        }
        total += val;
        if (val != (int)buf[i].iov_len) { //缓冲区中没有更多数据可读取
            break;
        }
    }
    return total;
}

int SSLSocket::recvFrom(void *buf, size_t length, const Address::ptr from, int flags) {
    WEBS_ASSERT(false);
    return -1;
}

int SSLSocket::recvFrom(iovec *buf, size_t length, const Address::ptr from, int flags) {
    WEBS_ASSERT(false);
    return -1;
}

std::ostream &SSLSocket::dump(std::ostream &os) const {
    os << "[SSLSocket sock = " << m_sock
       << ", is_connected" << m_isConnected
       << ", family = " << m_family
       << ", type = " << m_type
       << ", protocol = " << m_protocol;
    if (m_localAddress) {
        os << ", local_address = " << m_localAddress->toString();
    }
    if (m_remoteAddress) {
        os << ", remote_address = " << m_remoteAddress->toString();
    }
    return os << "]";
}

/* 服务器端创建SSL_CTX -- 加载SSL证书 -- 加载SSL私钥文件 -- 检查证书与私钥是否匹配 */
bool SSLSocket::loadCertificates(const std::string &cert_file, const std::string &key_file) {
    m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
    if (SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1) { // 加载 SSL 证书链文件到 SSL 上下文对象
        WEBS_LOG_DEBUG(g_logger) << "SSL_CTX_use_certificate_chain_file(" << cert_file << ") error";
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) { // 加载 SSL 私钥文件到 SSL 上下文对象
        WEBS_LOG_DEBUG(g_logger) << "SSL_CTX_use_PrivateKey_file(" << key_file << ") error";
        return false;
    }
    if (SSL_CTX_check_private_key(m_ctx.get()) != 1) { // 检查 SSL 上下文对象中的私钥与证书是否匹配
        WEBS_LOG_DEBUG(g_logger) << "SSL_CTX_check_private_key (cert_file = " << cert_file << ", key_file = " << key_file << ") error";
        return false;
    }
    return true;
}

/* 初始化sock -- 重置m_ssl -- 重新将m_sock与m_ssl绑定 -- 服务器端执行SSL握手 */
bool SSLSocket::init(int sock) {
    bool val = Socket::init(sock);
    if (val) {
        m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
        SSL_set_fd(m_ssl.get(), m_sock);
        // 调用 SSL_accept 函数，进行 SSL/TLS 握手。这是服务器端握手操作，它会等待并处理来自客户端的握手请求。
        val = (SSL_accept(m_ssl.get()) == 1);
    }
    return val;
}

std::ostream &operator<<(std::ostream &os, const Socket &sock) {
    return sock.dump(os);
}

} // namespace webs

int main() {
    return 0;
}