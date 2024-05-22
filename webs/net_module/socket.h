/* socket封装 */

#ifndef __WEBS_SOCKET_H__
#define __WEBS_SOCKET_H__

#include "address.h"
#include "../util_module/Noncopyable.h"

#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>

namespace webs {
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /* Socket类型 */
    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
    };

    /* Socket协议簇 */
    enum Famliy {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX,
    };

    /* 创建满足地址类型的 TCP地址 */
    static Socket::ptr CreateTCP(webs::Address::ptr address);

    /* 创建满足地址类型的 UDP地址 */
    static Socket::ptr CreateUDP(webs::Address::ptr address);

    /* 创建IPv4的TCP地址 */
    static Socket::ptr CreateTCPSocket();

    /* 创建IPv4的UDP地址 */
    static Socket::ptr CreateUDPSocket();

    /* 创建IPv6的TCP地址 */
    static Socket::ptr CreateTCPSocket6();

    /* 创建IPv6的UPD地址 */
    static Socket::ptr CreateUDPSocket6();

    /* 创建UNIX的TCP地址 */
    static Socket::ptr CreateUnixTCPSocket();

    /* 创建UNIX的UDP地址 */
    static Socket::ptr CreateUnixUDPSocket();

    /* 协议簇、socket类型(流式\数据包式)、具体的协议 */
    Socket(int family, int type, int protocol = 0);

    virtual ~Socket();

    /* 获取发送超时时间(毫秒) */
    int64_t getSendTimeout();

    /* 设置发送超时时间(毫秒) */
    void setSendTimeout(uint64_t v);

    /* 获取接受超时时间(毫秒) */
    uint64_t getRecvTimeout();

    /* 设置接收超时时间(毫秒) */
    void setRecvTimeout(uint64_t v);

    /* 获取sockopt; @see getsockopt(); optval输出参数 */
    bool getOption(int level, int optname, void *optval, socklen_t *optlen);

    /* getOption() 模板 --> 对getOption封装 */
    template <typename T>
    bool getOption(int level, int optname, T &value) {
        socklen_t len = sizeof(T);
        return getOption(level, optname, &value, &len);
    }

    /* 设置sockopt; @see setsockopt()；optval输入参数 */
    bool setOption(int level, int optname, const void *optval, socklen_t optlen);

    /* setOption() 模板 */
    template <typename T>
    bool setOption(int level, int optname, const T &optval) {
        return setOption(level, optname, &optval, sizeof(T));
    }

    /**
     * @brief 绑定监听套接字的地址
     * 
     * @param addr 
     * @return true 
     * @return false 
     */
    virtual bool bind(webs::Address::ptr addr);

    /**
     * @brief  监听套接字监听连接
     * 
     * @param backlog 未完成连接队列的最大长度
     * @return true 
     * @return false 
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 监听套接字接受连接
     * 
     * @return Socket::ptr 如果有连接到达，返回一个连接套接字
     */
    virtual Socket::ptr accept();

    /**
     * @brief 客户端发起连接
     * 
     * @param addr 目标地址
     * @param timeout_ms 超时时间
     * @return true 
     * @return false 
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    /**
     * @brief 客户端重新发起连接
     * 
     * @param timeout_ms 超时时间
     * @return true 
     * @return false 
     */
    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 关闭连接
     * 
     * @return true 
     * @return false 
     */
    virtual bool close();

    /**
     * @brief 向当前的socket发送数据 -- 用于TCP
     * 
     * @param buf 带发送数据的内存
     * @param length 带发送数据的大小
     * @param flags 标志： 是否有外带数据？是否绕过标准路由器检查？
     * @return int 
     *      @retval >0 成功；对应发送数据的大小
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int send(const void *buf, size_t length, int flags = 0);

    /**
     * @brief 向当前的socket发送数据 -- 用于TCP
     * 
     * @param buf 带发送数据的内存 ---> 适用于IO内存块
     * @param length 带发送数据的大小   Number of elements in the vector.
     * @param flags 标志
     * @return int 
     */
    virtual int send(const iovec *buf, size_t length, int flags = 0);

    /**
     * @brief 向当前的socket发送数据 --- 用于UDP
     * 
     * @param buf 带发送数据的内存
     * @param length 带发送数据的大小
     * @param to 接收端地址 --- 必须得指定数据的接收端，即数据目的地
     * @param flags 标志
     * @return int 
     */
    virtual int sendTo(const void *buf, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 向当前的socket发送数据 --- 用于UDP
     * 
     * @param buf 带发送数据的内存 ---> 适用于IO内存块
     * @param length 带发送数据的大小  Number of elements in the vector.
     * @param to 接收端地址 --- 必须得指定数据的接收端，即数据目的地
     * @param flags 标志
     * @return int 
     */
    virtual int sendTo(const iovec *buf, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 接收数据
     * 
     * @param buf 接收数据的内存位置 -- 是传出参数
     * @param length 内存的大小
     * @param flags 接收数据时的行为：如非堵塞（MSG_DONTWAIT）、接收紧急数据..
     * @return int 
     *      @retval >0 接收的数据大小
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recv(void *buf, size_t length, int flags = 0);

    /**
     * @brief 接收数据
     * 
     * @param buf 接收数据的内存位置 ---> 适用于缓冲区是IO内存块
     * @param length 内存的大小  Number of elements in the vector.
     * @param flags 接收数据时的行为：如非堵塞（MSG_DONTWAIT）、接收紧急数据..
     * @return int 
     */
    virtual int recv(iovec *buf, size_t length, int flags = 0);

    /**
     * @brief 从当前的socket读取数据 --- 用于UDP
     * 
     * @param buf 接收数据的内存
     * @param length 内存的大小
     * @param from 用来获取发送端地址 --- 数据来源
     * @param flags 
     * @return int 
     */
    virtual int recvFrom(void *buf, size_t length, const Address::ptr from, int flags = 0);

    /**
     * @brief 从当前的socket读取数据 -- 用于UDP
     * 
     * @param buf 接收数据的内存 ---> 适用于缓冲区是IO内存块
     * @param length 内存的大小  Number of elements in the vector.
     * @param from 用来获取发送端地址 --- 数据来源
     * @param flags 
     * @return int 
     */
    virtual int recvFrom(iovec *buf, size_t length, const Address::ptr from, int flags = 0);

    /**
     * @brief 输出信息到输出流
     * 
     * @param os 
     * @return std::ostream& 
     */
    virtual std::ostream &dump(std::ostream &os) const;

    /**
     * @brief 将输出流中的信息输出为字符串
     * 
     * @return std::string 
     */
    std::string toString() const;

    /**
     * @brief 获取远端地址；如果不存在，尝试获取
     * 
     * @return Address::ptr 
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址；如果不存在，尝试获取
     * 
     * @return Address::ptr 
     */
    Address::ptr getLocalAddress();

    /**
     * @brief 获取socket的协议簇
     * 
     * @return int 
     */
    int getFamily() const {
        return m_family;
    }

    /**
     * @brief 获取socket的服务类型 --流服务、数据报服务
     * 
     * @return int 
     */
    int getType() const {
        return m_type;
    }

    /**
     * @brief 获取socket的协议类型
     * 
     * @return int 
     */
    int getProtocol() const {
        return m_protocol;
    }

    /**
     * @brief Get the Socket object
     * 
     * @return int 
     */
    int getSocket() const {
        return m_sock;
    }

    /**
     * @brief 是否连接
     * 
     * @return true 
     * @return false 
     */
    bool isConnected() const {
        return m_isConnected;
    }

    /**
     * @brief socket是否有效
     * 
     * @return true 
     * @return false 
     */
    bool isVaild() const {
        return m_sock != -1;
    }

    /**
     * @brief Get the socket Error 
     * 
     * @return int 
     */
    int getError();

    /**
     * @brief 取消读
     * 
     * @return true 
     * @return false 
     */
    bool cancelRead();

    /**
     * @brief 取消写
     * 
     * @return true 
     * @return false 
     */
    bool cancelWrite();

    /**
     * @brief 取消accept
     * 
     * @return true 
     * @return false 
     */
    bool cancelAccept();

    /**
     * @brief 取消所有事件
     * 
     * @return true 
     * @return false 
     */
    bool cancelAll();

protected:
    /**
     * @brief 创建socket
     *  Socket的构造函数(Socket对象初始化) --> newSock(使用socket库函数，创建socketfd -- 对m_sock赋值) --> initSock初始化(设置socketfd的一些属性)
     */
    void newSock();

    /**
     * @brief 初始化socket
     * 
     */
    void initSock();

    /**
     * @brief 初始化socket  -- 用于派生类重写
     * 
     * @return true 
     * @return false 
     */
    virtual bool init(int sock);

protected: // 为什么这个数据的访问权限是protected？
    // socket的文件描述符
    int m_sock;
    // 协议簇
    int m_family;
    // 类型
    int m_type;
    // 具体的协议
    int m_protocol;
    // 是否连接
    bool m_isConnected;
    // 本地地址
    Address::ptr m_localAddress;
    // 远端地址
    Address::ptr m_remoteAddress;
};

class SSLSocket : public Socket {
public:
    typedef std::shared_ptr<SSLSocket> ptr;

    /* 创建满足地址类型的 TCP地址 */
    static SSLSocket::ptr CreateTCP(Address::ptr address);

    /* 创建IPv4的TCP地址 */
    static SSLSocket::ptr CreateTCPSocket();

    /* 创建IPv6的TCP地址 */
    static SSLSocket::ptr CreateTCPSocket6();

    SSLSocket(int family, int type, int protocol = 0);

    virtual bool bind(webs::Address::ptr addr) override;

    virtual bool listen(int backlog = SOMAXCONN) override;

    virtual Socket::ptr accept() override;

    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;

    // virtual bool reconnect(uint64_t timeout_ms = -1) override;

    virtual bool close() override;

    virtual int send(const void *buf, size_t length, int flags = 0) override;

    virtual int send(const iovec *buf, size_t length, int flags = 0) override;

    virtual int sendTo(const void *buf, size_t length, const Address::ptr to, int flags = 0) override;

    virtual int sendTo(const iovec *buf, size_t length, const Address::ptr to, int flags = 0) override;

    virtual int recv(void *buf, size_t length, int flags = 0) override;

    virtual int recv(iovec *buf, size_t length, int flags = 0) override;

    virtual int recvFrom(void *buf, size_t length, const Address::ptr from, int flags = 0) override;

    virtual int recvFrom(iovec *buf, size_t length, const Address::ptr from, int flags = 0) override;

    virtual std::ostream &dump(std::ostream &os) const override;

    bool loadCertificates(const std::string &cert_file, const std::string &key_file);

protected:
    virtual bool init(int sock) override;

private:
    std::shared_ptr<SSL_CTX> m_ctx;
    std::shared_ptr<SSL> m_ssl;
};

std::ostream &operator<<(std::ostream &os, const Socket &sock);
} // namespace webs

#endif
