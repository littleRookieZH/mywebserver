#include "tcp_server.h"
#include <functional>

namespace webs {
// 日志
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

// read_timeout 配置 信息
static webs::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = webs::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

TcpServer::TcpServer(webs::IOManager *worker, webs::IOManager *io_worker, webs::IOManager *accept_worker) :
    m_worker(worker),
    m_ioWorker(io_worker),
    m_acceptWorker(accept_worker),
    m_recvTimeout(g_tcp_server_read_timeout->getValue()),
    m_name("webs/1.0.0"),
    m_isStop(true) {
}

TcpServer::~TcpServer() {
    for (auto &i : m_socks) {
        i->close(); // 关闭m_socketfd
    }
    m_socks.clear(); // 清空vector -- 这里应该会自动清理
}

bool TcpServer::bind(webs::Address::ptr address, bool ssl) {
    std::vector<webs::Address::ptr> addr{address};
    std::vector<webs::Address::ptr> fails;
    return bind(addr, fails, ssl);
}

/* 将一个地址(服务器的ip和指定端口)绑定到socketfd -- 创建socket -- 绑定 -- 加入监听队列 -- 输出日志  */
bool TcpServer::bind(const std::vector<webs::Address::ptr> &addrs, std::vector<webs::Address::ptr> &fails, bool ssl) {
    m_ssl = ssl;
    for (auto &addr : addrs) {
        Socket::ptr sock = ssl ? webs::SSLSocket::CreateTCP(addr) : webs::Socket::CreateTCP(addr);
        if (!sock->bind(addr)) {
            WEBS_LOG_ERROR(g_logger) << "bind fail errno = " << errno << "errstr = " << strerror(errno)
                                     << " addr = [" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()) {
            WEBS_LOG_ERROR(g_logger) << "listen fail errno = " << errno << "errstr = " << strerror(errno)
                                     << " addr = [" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }

    if (!fails.empty()) { // 如果失败，清除已经连接的socketfd
        m_socks.clear();
        return false;
    }

    for (auto &i : m_socks) {
        WEBS_LOG_INFO(g_logger) << "type = " << m_type
                                << ", name =" << m_name << ", ssl = " << m_ssl
                                << ", server bind success: " << *i;
    }
    return true;
}
/* 启动TcpServer是指：将监听socket开始等待接受新连接这项工作加入到协程调度器中 */
bool TcpServer::start() {
    if (!m_isStop) {
        return false; // 返回false表示服务器已经在运行
    }
    m_isStop = false;
    for (auto &sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

/* 将 m_socks中的sock取消所有事件，并关闭socketfd 这项任务添加到协程管理器中 */
void TcpServer::stop() {
    if (m_isStop) {
        return;
    }
    m_isStop = true;
    auto self = shared_from_this();
    // 通过捕获self，你可以确保在lambda函数执行期间，TcpServer对象不会被销毁。
    // 即使在stop()函数返回后，TcpServer对象也会一直存在，直到lambda函数执行完毕。
    m_acceptWorker->schedule([this, self]() {
        for (auto &sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::setConf(const TcpServerConf &conf) {
    m_conf.reset(new TcpServerConf(conf));
}

void TcpServer::handleClient(Socket::ptr client) {
    WEBS_LOG_INFO(g_logger) << "handleClient: " << *client;
}

/* 该函数与sock接受一个新连接的处理逻辑相同: 接受连接 -- 将工作任务加入调度器中 */
void TcpServer::startAccept(Socket::ptr sock) {
    while (!m_isStop) {
        Socket::ptr client = sock->accept();
        if (client) {
            client->setRecvTimeout(m_recvTimeout);
            // handleClient处理客户端请求的工作任务 -- 派生类需要重载的函数
            // shared_from_this(): 获取调用该成员函数的对象的 std::shared_ptr；增加了引用计数，保证了不会发生执行工作函数时对象不存在的情况
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        } else {
            WEBS_LOG_ERROR(g_logger) << "accept errno = " << errno << " errstr = " << strerror(errno);
        }
    }
}

/* 强转 -- 如果成功加载证书和密钥 */
bool TcpServer::loadCertificates(const std::string &cert_file, const std::string &key_file) {
    for (auto &sock : m_socks) {
        auto sslSock = std::dynamic_pointer_cast<SSLSocket>(sock);
        if (sslSock) {
            if (!sslSock->loadCertificates(cert_file, key_file)) {
                return false;
            }
        }
    }
    return true;
}

/* 输出对象的属性 */
std::string TcpServer::toString(const std::string &prefix) {
    std::stringstream ss;
    ss << prefix << "[type = " << m_type << ", name = " << m_name
       << ", worker = " << (m_worker ? m_worker->getName() : "")
       << ", ssl = " << m_ssl
       << ", io_worker = " << (m_ioWorker ? m_ioWorker->getName() : "")
       << ", accept_worker = " << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << ", recv_timeout = " << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for (auto &i : m_socks) { // 输出两次前缀pfx是为了增加缩进，使得输出的信息更加清晰和易读。
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}

} // namespace webs