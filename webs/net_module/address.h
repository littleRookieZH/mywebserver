#ifndef __WEBS_ADDRESS_H__
#define __WEBS_ADDRESS_H__

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <vector>
#include <string>
#include <map>

namespace webs {

class IPAddress;
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    /* 通过sockaddr指针创建Address */
    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

    /* 获取host地址的所有Address */
    static bool Lookup(std::vector<Address::ptr> &result, const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);

    /* 获取host地址的任意Address */
    static Address::ptr LookupAny(const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);

    /* 获取host地址的任意IPAddress */
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);

    /* 返回本机所有网卡的 <网卡名， <地址，子网掩码位数>>*/
    static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result, int family = AF_INET);

    /* 返回指定网卡名的地址和子网掩码数 */
    static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family = AF_INET);

    virtual ~Address() {
    }

    /* 返回协议簇 */
    int getFamily() const;

    /* 返回sockaddr指针，只读 */
    virtual const sockaddr *getAddr() const = 0;

    /* 返回sockaddr指针，读写 */
    virtual sockaddr *getAddr() = 0;

    /* 返回sockaddr的长度 */
    virtual socklen_t getAddrlen() const = 0;

    /* 可读性输出地址 */
    virtual std::ostream &insert(std::ostream &os) const = 0;

    /* 返回可读性字符串 */
    std::string toString() const;

    /* 为了Address可以用于容器 */
    bool operator<(Address &rhs) const;

    bool operator==(Address &rhs) const;

    bool operator!=(Address &rhs) const;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    /* 通过ip字符串，创建IPAddress */
    static IPAddress::ptr Create(const char *address, uint16_t port = 0);

    /* 获取该地址的广播地址 */
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

    /* 获取该地址的网段 -- 网络地址 */
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;

    /* 获取该地址的子网掩码 */
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    /* 设置端口号 */
    virtual void setPort(uint16_t v) = 0;

    /* 获取端口号 */
    virtual uint32_t getPort() const = 0;
};

class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    IPv4Address(const sockaddr_in &addr);

    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    /* 接受点分十进制的IP4地址，端口 */
    static IPv4Address::ptr Create(const char *addr, uint16_t port = 0);

    /* 返回sockaddr指针，只读 */
    const sockaddr *getAddr() const override;

    /* 返回sockaddr指针，读写 */
    sockaddr *getAddr() override;

    /* 返回sockaddr的长度 */
    socklen_t getAddrlen() const override;

    /* 可读性输出地址 */
    std::ostream &insert(std::ostream &os) const override;

    /* 获取该地址的广播地址 */
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    /* 获取该地址的网段 -- 网络地址 */
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;

    /* 获取该地址的子网掩码 */
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    /* 设置端口号 */
    void setPort(uint16_t v) override;

    /* 获取端口号 */
    uint32_t getPort() const override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    IPv6Address();

    IPv6Address(const sockaddr_in6 &addr);

    IPv6Address(const uint8_t addr[16], uint16_t port = 0);

    /* 通过IP6的字符串地址构造IPv6Address */
    static IPv6Address::ptr Create(const char *addr, uint16_t port = 0);

    /* 返回sockaddr指针，只读 */
    const sockaddr *getAddr() const override;

    /* 返回sockaddr指针，读写 */
    sockaddr *getAddr() override;

    /* 返回sockaddr的长度 */
    socklen_t getAddrlen() const override;

    /* 可读性输出地址 */
    std::ostream &insert(std::ostream &os) const override;

    /* 获取该地址的广播地址 */
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    /* 获取该地址的网段 -- 网络地址 */
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;

    /* 获取该地址的子网掩码 */
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    /* 设置端口号 */
    void setPort(uint16_t v) override;

    /* 获取端口号 */
    uint32_t getPort() const override;

private:
    sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string &path);

    /* 返回sockaddr指针，只读 */
    virtual const sockaddr *getAddr() const;

    /* 返回sockaddr指针，读写 */
    virtual sockaddr *getAddr();

    /* 返回sockaddr的长度 */
    virtual socklen_t getAddrlen() const;

    /* 可读性输出地址 */
    virtual std::ostream &insert(std::ostream &os) const;

    /* 设置sockaddr的长度 */
    void setAddrlen(uint32_t v);

    /* 返回路径 */
    std::string getPath() const;

private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);

    UnknownAddress(const sockaddr &addr);

    /* 返回sockaddr指针，只读 */
    virtual const sockaddr *getAddr() const;

    /* 返回sockaddr指针，读写 */
    virtual sockaddr *getAddr();

    /* 返回sockaddr的长度 */
    virtual socklen_t getAddrlen() const;

    /* 可读性输出地址 */
    virtual std::ostream &insert(std::ostream &os) const;

private:
    sockaddr m_addr;
};

/* 流式输出Address */
std::ostream &operator<<(std::ostream &os, const Address &addr);

} // namespace webs

#endif