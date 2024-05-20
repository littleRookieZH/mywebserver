#include <netdb.h>
#include <string.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "address.h"
#include "../log_module/log.h"
#include "../util_module/endian.h"

namespace webs {
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

/* 计算掩码 */
template <typename T>
static T CreateMask(uint32_t bits) { // 前bits位全是0，后面的位全是1
    return 1 << (sizeof(T) * 8 - bits) - 1;
}

/* 计算掩码的位数：先获取掩码 --> 再使用该方法 */
template <typename T>
static uint32_t CountBytes(T value) {
    int result = 0;
    for (; value; ++result) {
        value &= value - 1;
    }
    return result;
}

/* 获取host地址的任意Address */
Address::ptr Address::LookupAny(const std::string &host, int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

/* 获取host地址的任意IPAddress */
std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string &host, int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        for (auto &i : result) {
            // dynamic_pointer_cast 返回nullptr 或者 所需的ptr
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }
    return nullptr;
}

/* 获取host地址的所有Address */
/**
 * 假定解析的格式为：
 * [ ipv6 ]:端口号
 * ip6 : 端口号
 * 如果都不是假定输入的是：ip地址
 * 
 * 设置想要获取的地址信息格式  -->  解析格式 --> 获取对应的地址信息  --> 如果地址信息存在，将其添加到结果中
 * */
bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host, int family, int type, int protocol) {
    struct addrinfo hints, *results, *next;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_family = family;
    hints.ai_flags = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char *service = NULL;
    if (!host.empty() && host[0] == '[') { // 格式：[ ipv6 ]:端口号
        const char *endipv6 = (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
        if (*(endipv6 + 1) == ':') { // ip6的下一个字符为:  类似： [XXXX]:
            service = endipv6 + 2;   // service指向端口号的位置
        }
        node = host.substr(1, endipv6 - host.c_str() - 1); // 获取ip6的字符串
    }

    if (node.empty()) { // 解析格式：ip6 : 端口号
        service = (const char *)memchr(host.c_str(), ':', host.size());
        if (service) {
            if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) { // 没有找到下一个 : 字符
                ++service;
                node = host.substr(0, service - host.c_str());
            }
        }
    }

    if (node.empty()) { // 如果都不符合条件，假定输入的是 ip地址
        node = host;
    }

    // getaddrinfo() 或 getnameinfo() 函数出现错误时，可以使用 gai_strerror 来获取对应的错误信息
    int error = getaddrinfo(node.c_str(), service, &hints, &results); // 成功：0；失败：非0
    if (error) {
        WEBS_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host
                                 << ", " << family << ", " << type << ") error = " << error
                                 << "errstr = " << gai_strerror(error);
        return false;
    }
    // 将结果加入到result
    next = results;
    while (next) {
        result.push_back(Create(next->ai_addr, next->ai_addrlen));
        next = next->ai_next;
    }
    freeaddrinfo(results);
    return !result.empty();
}

/* 返回本机所有网卡的 <网卡名， <地址，子网掩码位数>> */
/**
 * 获取网络接口信息 -->  family是否正确 --> 网卡的协议为：ip4 -- 创建地址，获取子网掩码，掩码长度 / ip6 -- 创建地址，获取子网掩码，掩码长度 
 * 收集结果  
*/
bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result, int family) {
    struct ifaddrs *next, *results;
    if (getifaddrs(&results) != 0) {
        WEBS_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs  err = "
                                 << errno << " errstr = " << strerror(errno);
        return false;
    }
    try {
        for (next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch (next->ifa_addr->sa_family) {
            case AF_INET: {
                addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                uint32_t umask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                prefix_len = CountBytes(umask);
            } break;
            case AF_INET6: {
                addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                in6_addr &umask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
                prefix_len = 0;
                for (int i = 0; i < 16; ++i) {
                    prefix_len += CountBytes(umask.s6_addr[i]);
                }
            } break;
            default:
                break;
            }
            if (addr) {
                // result.insert(std::pair<std::string, std::pair<Address::ptr, uint32_t>>({next->ifa_name, {addr, prefix_len}}));
                // result.insert({next->ifa_name, {addr, prefix_len}});
                result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        WEBS_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception ";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
}

/* 返回指定网卡名的地址和子网掩码数 */
/* 如果 iface 为空 或者 * -->  family == AF_INET || AF_UNPEC? -- 加入ip4 ； family == AF_INET6 || AF_UNPEC? -- 加入ip6
    获取本机所有的网卡地址 --> equal_range -- 获取指定iface的地址
*/
bool Address::GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family) {
    if (iface.empty() || iface == "*") {
        if (family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address), 0u));
        }
        if (family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address), 0u));
        }
    }
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
    if (!GetInterfaceAddresses(results, family)) {
        return false;
    }
    // 返回一个 pair 类型值；第 1 个迭代器指向的是 [first, last) 区域中第一个等于 val 的元素；第 2 个迭代器指向的是 [first, last) 区域中第一个大于 val 的元素
    auto it = results.equal_range(iface);
    for (it.first; it.first != it.second; ++it.first) {
        result.push_back(it.first->second);
    }
    return !result.empty();
}

int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() const {
    // 非抽线函数调用抽象函数不会编译错误吗？
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

/* 逐字符比较两个地址的大小 */
bool Address::operator<(Address &rhs) const {
    socklen_t min_len = std::min(getAddrlen(), rhs.getAddrlen());
    int result = memcmp(getAddr(), rhs.getAddr(), min_len);
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else {
        return getAddrlen() < rhs.getAddrlen() ? true : false;
    }
}

bool Address::operator==(Address &rhs) const {
    return getAddrlen() == rhs.getAddrlen() && memcmp(getAddr(), rhs.getAddr(), getAddrlen()) == 0;
}

bool Address::operator!=(Address &rhs) const {
    return !(*this == rhs);
}

/* 分类型创建地址； 为什么可以在抽象类中使用了派生类的构造函数 ---> 在使用之前该类已经被定义出来了 */
Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if (addr == nullptr) {
        return nullptr;
    }
    Address::ptr result;
    switch (addr->sa_family) {
    case AF_INET:
        result.reset(new IPv4Address(*(sockaddr_in *)addr)); // * 优先级与 ()强制转换同一级
        break;
    case AF_INET6:
        result.reset(new IPv6Address(*(sockaddr_in6 *)addr));
        break;
    default:
        result.reset(new UnknownAddress(*addr));
        break;
    }
}

/**
 * 只允许输入点分十进制的字符型Ip地址
 * 输入地址和端口 --> 使用getaddrinfo获取socket地址信息 --> 调用create根据类型创建连接地址 ---> 
 * 根据socket地址信息中的协议类型选择对应的构造函数，初始化地址信息
 * 就是给 socketaddr_in / socketaddr_in6赋值
*/
IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
    if (address == nullptr) {
        return nullptr;
    }
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, NULL, &hints, &results);
    if (error) {
        WEBS_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
                                 << ", " << port << ") error = " << error << "errno = " << errno
                                 << "errstr = " << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr rt = std::dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if (rt) {
            rt->setPort(byteswapOnLittleEndian(port));
        }
        freeaddrinfo(results);
        return rt;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

IPv4Address::IPv4Address(const sockaddr_in &addr) {
    m_addr = addr; // 元素内容会挨个拷贝；不是引用。。。
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(sockaddr_in));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
    m_addr.sin_port = byteswapOnLittleEndian(port);
}

/* 使用点分十进制地址创建IPv4Address，不能是主机名 */
/* 使用inet_pton转为网络字节序的地址 */
IPv4Address::ptr IPv4Address::Create(const char *addr, uint16_t port) {
    if (addr == nullptr) {
        return nullptr;
    }
    IPv4Address::ptr result(new IPv4Address);
    int rt = inet_pton(AF_INET, addr, &result->m_addr.sin_addr);
    if (rt <= 0) { // 正数表示成功
        WEBS_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << addr
                                 << ", " << port << ") rt = " << rt << " errno = "
                                 << errno << "errstr = " << strerror(errno);
        return nullptr;
    }
    result->m_addr.sin_port = byteswapOnLittleEndian(port);
    return result;
}

/* 获取socket地址；不是ip地址信息 */
const sockaddr *IPv4Address::getAddr() const {
    return (sockaddr *)&m_addr;
}

/* 获取socket地址；不是ip地址信息 */
sockaddr *IPv4Address::getAddr() {
    return (sockaddr *)&m_addr;
}

/* 获取socket地址长度 */
socklen_t IPv4Address::getAddrlen() const {
    return sizeof(m_addr);
}

/* 可读性输出地址；格式：点分十进制的ip:port */
std::ostream &IPv4Address::insert(std::ostream &os) const {
    uint32_t ipaddress = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    // 0xff 高位清 0;
    os << ((ipaddress >> 24) & 0xff) << "."
       << ((ipaddress >> 16) & 0xff) << "."
       << ((ipaddress >> 8) & 0xff) << "."
       << (ipaddress & 0xff) << ":" << byteswapOnLittleEndian(m_addr.sin_port);

    return os;
}

/* 获取该地址的广播地址 */
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    // 获取掩码
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

/* 获取该地址的网段 -- 网络地址 */
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in naddr(m_addr);
    // 获取掩码 --> 转为网络字节序 --> 取反获取子网掩码 --> 取与获取网段地址 （IP地址：网络号(网段)+主机号）
    naddr.sin_addr.s_addr &= ~(byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len)));
    return IPv4Address::ptr(new IPv4Address(naddr));
}

/* 获取该地址的子网掩码 */
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in subaddr;
    subaddr.sin_family = AF_INET;
    subaddr.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len)); // 子网掩码 前prefix_len为1，后面的位为0
    return IPv4Address::ptr(new IPv4Address(subaddr));
}

/* 设置端口号 */
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

/* 获取端口号 */
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(sockaddr_in6));
    m_addr.sin6_family = AF_INET6; // memset后，默认端口为0
}

IPv6Address::IPv6Address(const sockaddr_in6 &addr) {
    m_addr = addr;
}

IPv6Address::IPv6Address(const uint8_t addr[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    m_addr.sin6_family = AF_INET6;
    memcmp(&m_addr.sin6_addr.s6_addr, addr, 16);
}

/* 通过IP6的字符串地址构造IPv6Address */
IPv6Address::ptr IPv6Address::Create(const char *addr, uint16_t port) {
    if (addr == nullptr) {
        return nullptr;
    }
    IPv6Address::ptr naddr(new IPv6Address);
    int rt = inet_pton(AF_INET6, addr, &naddr->m_addr.sin6_addr); // 因为是静态函数，所以可以naddr可以访问m_addr
    if (rt <= 0) {
        WEBS_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << addr << ", " << port
                                 << ") rt = " << rt << "errno = " << errno << "errstr = " << strerror(errno);
        return nullptr;
    }
    naddr->m_addr.sin6_port = byteswapOnLittleEndian(port);
    return naddr;
}

/* 返回sockaddr指针，只读 */
const sockaddr *IPv6Address::getAddr() const {
    return (sockaddr *)&m_addr;
}

/* 返回sockaddr指针，读写 */
sockaddr *IPv6Address::getAddr() {
    return (sockaddr *)&m_addr;
}

/* 返回sockaddr的长度 */
socklen_t IPv6Address::getAddrlen() const {
    return sizeof(m_addr);
}

/* 获取端口号 */
uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

/**
 * 可读性输出地址；
 * 输出ipv6地址，0会发生合并；比如  1231::2100
 * 1221:1212:2211:...
 */
std::ostream &IPv6Address::insert(std::ostream &os) const {
    bool start = false;
    uint16_t *addrs = (uint16_t *)m_addr.sin6_addr.s6_addr16;
    os << '[';
    for (int i = 0; i < 8; ++i) {
        if (addrs[i] == 0 && !start) {
            continue;
        }
        if (addrs[i] && addrs[i - 1] == 0 && !start) {
            os << ':';
            start = true;
        }
        if (i) {
            os << ':';
        }
        os << std::hex << byteswapOnLittleEndian(addrs[i]) << std::dec;
    }
    if (!start && addrs[7] == 0) {
        os << "::";
    }
    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

/* 感觉代码有问题，先不写 */
/* 获取该地址的组播地址 */
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
}

/* 获取该地址的网段 -- 网络地址 */
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
}

/* 获取该地址的子网掩码 */
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
}

void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

std::ostream &operator<<(std::ostream &os, const Address &addr) {
    return addr.insert(os);
}

// 最大路径长度
static const size_t MAX_PAHT_LEN = (sizeof((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    // 返回 member(结构体中的成员名称) 在 type(结构体类型的名称) 结构体中的偏移量（以字节为单位）。
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PAHT_LEN;
}

/* 如果是抽象路径(以\0开头)：m_length - 1;*/
UnixAddress::UnixAddress(const std::string &path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_INET6;
    m_length = path.size() + 1;

    if (!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if (m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(&m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

/* 返回sockaddr指针，只读 */
const sockaddr *UnixAddress::getAddr() const {
    return (sockaddr *)&m_addr;
}

/* 返回sockaddr指针，读写 */
sockaddr *UnixAddress::getAddr() {
    return (sockaddr *)&m_addr;
}

/* 返回sockaddr的长度 */
socklen_t UnixAddress::getAddrlen() const {
    return m_length;
}

/* 可读性输出地址 */
std::ostream &UnixAddress::insert(std::ostream &os) const {
    uint32_t length = m_length - offsetof(sockaddr_un, sun_path);
    if (length && m_addr.sun_path[0] == '\0') {          // 是抽象数据
        std::string s1(m_addr.sun_path + 1, length - 1); // 如果将\0包含在一起使用构造，会产生空字符串
        return os << "\\0" << s1;
    }
    return os << m_addr.sun_path;
}

/* 设置sockaddr的长度 */
void UnixAddress::setAddrlen(uint32_t v) {
    m_length = v;
}

/* 返回路径 */
std::string UnixAddress::getPath() const {
    uint32_t len = m_length - offsetof(sockaddr_un, sun_path);
    std::stringstream ss;
    if (len && m_addr.sun_path[0] == '\0') {                      // 是抽象数据
        ss << "\\0" << std::string(m_addr.sun_path + 1, len - 1); // 如果将\0包含在一起使用构造，会产生空字符串
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr &addr) {
    m_addr = addr;
}

/* 返回sockaddr指针，只读 */
const sockaddr *UnknownAddress::getAddr() const {
    return &m_addr;
}

/* 返回sockaddr指针，读写 */
sockaddr *UnknownAddress::getAddr() {
    return &m_addr;
}

/* 返回sockaddr的长度 */
socklen_t UnknownAddress::getAddrlen() const {
    return sizeof(m_addr);
}

/* 可读性输出地址 */
std::ostream &UnknownAddress::insert(std::ostream &os) const {
    return os << "[UnknownAddress family = " << m_addr.sa_family << "]";
}

} // namespace webs

// int main() {
//     return 0;
// }